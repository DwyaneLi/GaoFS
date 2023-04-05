//
// Created by DELL on 2023/2/28.
//

#include <daemon/handler/rpc_util.hpp>
#include <daemon/handler/rpc_defs.hpp>
#include <daemon/ops/metadentry.hpp>
#include <daemon/backend/metadata/db.hpp> // exception
#include <common/rpc/rpc_types.hpp>
#include <daemon/ops/data.hpp>
#include <common/statistics/stats.hpp>
using namespace std;

namespace {

// 创建KV store（rockdb）的条目，如果当前条目已经存在，就返回一个错误。
// 所有的异常都要在这里发现并且捕获
hg_return_t rpc_srv_create(hg_handle_t handle) {
    // 初始化输入输出参数
    rpc_err_out_t out;
    rpc_mk_node_in_t in;
    auto ret = margo_get_input(handle, &in);
    if(ret != HG_SUCCESS) {
        GAOFS_DATA->spdlogger()->error(
                "{}() Failed to retrieve input from handle",
                __func__);
    }
    assert(ret == HG_SUCCESS);
    GAOFS_DATA->spdlogger()->debug(
            "{}() Got RPC with path '{}'",
            __func__, in.path);

    // 向rocksdb中添加metadata
    gaofs::metadata::Metadata md(in.mode);
    try {
        gaofs::metadata::create(in.path, md);
        out.err = 0;
    } catch (const gaofs::db_exception::ExistsException& e) {
        out.err = EEXIST;
    } catch (const std::exception& e) {
        GAOFS_DATA->spdlogger()->error(
                "{}() Failed to create metadentry: '{}'",
                __func__, e.what());
        out.err = -1;
    }

    GAOFS_DATA->spdlogger()->debug("{}() Sending output err '{}'",
                                   __func__, out.err);
    // 因为太简单，所以不用使用自己的respond和cleanup
    auto hret = margo_respond(handle, &out);
    if(hret != HG_SUCCESS) {
        GAOFS_DATA->spdlogger()->error(
                "{}() Failed to respond",
                __func__);
    }

    // 释放占有的rpc资源
    margo_free_input(handle, &in);
    margo_destroy(handle);

    // statistics
    if(GAOFS_DATA->enable_stats()) {
        GAOFS_DATA->stats()->add_value_iops(
                gaofs::utils::Stats::IopsOp::iops_create);
    }
    return HG_SUCCESS;
}

//统计请求读取KV存储中相应的条目。值字符串直接传递给客户端。
//如果对象不存在或出现其他意外错误，则设置错误代码
//其实就是返回db里已经被序列化的metadata，在client那边可以用这个值回复成metadata结构
hg_return_t rpc_srv_stat(hg_handle_t handle) {
    // 初始化输入输出结构
    rpc_path_only_in_t in{};
    rpc_stat_out_t out{};
    auto ret = margo_get_input(handle, &in);
    if(ret != HG_SUCCESS) {
        GAOFS_DATA->spdlogger()->error(
                "{}() Failed to retrieve input from handle",
                __func__);
    }
    assert(ret == HG_SUCCESS);
    GAOFS_DATA->spdlogger()->debug(
            "{}() path: '{}'",
            __func__, in.path);
    std::string val;

    // 实际操作
    try {
        val = gaofs::metadata::get_str(in.path);
        out.err = 0;
        out.db_val = val.c_str();
    } catch (const gaofs::db_exception::NotFoundException& e) {
        GAOFS_DATA->spdlogger()->debug("{}() Entry not found: '{}'", __func__, in.path);
        out.err = ENOENT;
    } catch (const std::exception& e) {
        GAOFS_DATA->spdlogger()->error("{}() Failed to get metadentry from DB: '{}'", __func__, e.what());
    }

    GAOFS_DATA->spdlogger()->debug("{}() Sending output mode '{}'",
                __func__, out.db_val);

    // 传输输出
    auto hret = margo_respond(handle, &out);
    if(hret != HG_SUCCESS) {
        GAOFS_DATA->spdlogger()->error("{}() Failed to respond", __func__);
    }

    // 释放资源
    margo_free_input(handle, &in);
    margo_destroy(handle);

    //statistics
    if(GAOFS_DATA->enable_stats()) {
        GAOFS_DATA->stats()->add_value_iops(
                gaofs::utils::Stats::IopsOp::iops_stats);
    }
    return HG_SUCCESS;
}


// 处理减少file size的请求
hg_return_t rpc_srv_decr_size(hg_handle_t handle) {
    // 初始化输入输出参数
    rpc_trunc_in_t in{};
    rpc_err_out_t out{};

    auto ret = margo_get_input(handle, &in);
    if(ret != HG_SUCCESS) {
        GAOFS_DATA->spdlogger()->error(
                "{}() Failed to retrieve input from handle",
                __func__);
        throw runtime_error("Failed to retrieve input from handle");
    }

    GAOFS_DATA->spdlogger()->debug(
            "{}() path: '{}', length: '{}'",
            __func__, in.path, in.length);

    // 进行操作，decrease没有进行ops的封装
    try {
        GAOFS_DATA->mdb()->decrease_size(in.path, in.length);
        out.err = 0;
    } catch (const std::exception& e) {
        GAOFS_DATA->spdlogger()->error(
                "{}() Failed to decrease size: '{}'",
                __func__, e.what());
        out.err = EIO;
    }

    // respond结果
    GAOFS_DATA->spdlogger()->debug(
            "{}() Sending output '{}'",
            __func__, out.err);
    auto hret = margo_respond(handle, &out);
    if(hret != HG_SUCCESS) {
        GAOFS_DATA->spdlogger()->error(
                "{}() Failed to respond",
                __func__);
        throw runtime_error("Failed to respond");
    }

    // 释放资源
    margo_free_input(handle, &in);
    margo_destroy(handle);
    return HG_SUCCESS;
}

// 处理程序触发KV存储条目的删除，但仍然向客户机返回文件模式和大小信息。这是因为需要大小来删除所有数据块。
// 首先删除元数据，以确保在元数据仍然可用时数据不会被删除。这可能会导致问题，因为一个统计请求会说文件仍然存在。
// gaofs::config::metadata::implicit_data_removal提供了隐式删除元数据节点上数据块的优化。这可以提高小文件的删除性能。
// 所以这里会删first chunk
hg_return_t rpc_srv_remove_metadata(hg_handle_t handle) {
    // 初始化输入输出参数
    rpc_rm_node_in_t in{};
    rpc_rm_metadata_out_t out{};
    auto ret = margo_get_input(handle, &in);
    if(ret != HG_SUCCESS) {
        GAOFS_DATA->spdlogger()->error(
                "{}() Failed to retrieve input from handle",
                __func__);
    }
    assert(ret == HG_SUCCESS);
    GAOFS_DATA->spdlogger()->debug(
            "{}() Got remove metadata RPC with path '{}'",
            __func__, in.path);

    // 具体操作
    try {
        // 因为要获取size 和 mode，所以要获取
        auto md = gaofs::metadata::get(in.path);
        gaofs::metadata::remove(in.path);
        out.err = 0;
        out.mode = md.mode();
        out.size = md.size();
        if constexpr(gaofs::config::metadata::implicit_data_removal) {
            if(S_ISREG(md.mode()) && (md.size()) != 0) {
                // 删除第一块 firstchunk
                gaofs::metadata::remove_first_chunk(in.path);
                GAOFS_DATA->storage()->destroy_chunk_space(in.path);
            }
        }
    } catch (const gaofs::db_exception::DBException& e) {
        out.err = EIO;
        GAOFS_DATA->spdlogger()->error(
                "{}(): path '{}' message '{}'",
                __func__, in.path, e.what());
    } catch (const gaofs::data::ChunkStorageException& e) {
        out.err = e.code().value();
        GAOFS_DATA->spdlogger()->error(
                "{}(): path '{}' errcode '{}' message '{}'",
                __func__, in.path, e.code().value(), e.what());
    } catch (const std::exception& e) {
        out.err = EBUSY;
        GAOFS_DATA->spdlogger()->error(
                "{}() path '{}' message '{}'",
                __func__, in.path, e.what());
    }

    GAOFS_DATA->spdlogger()->debug(
            "{}() Sending output '{}'",
            __func__, out.err);
    // 传输结果
    auto hret = margo_respond(handle, &out);
    if(hret != HG_SUCCESS) {
        GAOFS_DATA->spdlogger()->error(
                "{}() Failed to respond",
                __func__);
    }
    // 释放资源
    margo_free_input(handle, &in);
    margo_destroy(handle);
    // statistics
    if(GAOFS_DATA->enable_stats()) {
        GAOFS_DATA->stats()->add_value_iops(
                gaofs::utils::Stats::IopsOp::iops_remove);
    }
    return HG_SUCCESS;
}


// 处理程序只是发出删除本地文件系统上所有块文件的命令
// 如果有隐式删除的config，这个在之前在remove metadata里就执行了。
// 这个和metadata没关系
// 这里应该也不需要对first chunk进行处理，因为在上面已经删掉了
hg_return_t rpc_srv_remove_data(hg_handle_t handle) {
    // first chunk
    // 初始化输入输出
    rpc_rm_node_in_t in{};
    rpc_err_out_t out{};
    auto ret = margo_get_input(handle, &in);
    if(ret != HG_SUCCESS) {
        GAOFS_DATA->spdlogger()->error(
                "{}() Failed to retrieve input from handle",
                __func__);
    }
    assert(ret == HG_SUCCESS);
    GAOFS_DATA->spdlogger()->debug(
            "{}() Got remove data RPC with path '{}'",
            __func__, in.path);

    // 执行具体操作
    try {
        // 也要删除first chunk
        gaofs::metadata::remove_first_chunk(in.path);
        GAOFS_DATA->storage()->destroy_chunk_space(in.path);
        out.err = 0;
    } catch (const gaofs::db_exception::DBException& e) {
        out.err = EIO;
        GAOFS_DATA->spdlogger()->error(
                "{}(): path '{}' message '{}'",
                __func__, in.path, e.what());
    } catch (const gaofs::data::ChunkStorageException& e) {
        out.err = e.code().value();
        GAOFS_DATA->spdlogger()->error(
                "{}(): path '{}' errcode '{}' message '{}'",
                __func__, in.path, e.code().value(), e.what());
    } catch (std::exception& e) {
        out.err = EBUSY;
        GAOFS_DATA->spdlogger()->error(
                "{}() path '{}' message '{}'",
                __func__, in.path, e.what());
    }

    GAOFS_DATA->spdlogger()->debug(
            "{}() Sending output '{}'",
            __func__, out.err);
    // 传输输出
    auto hret = margo_respond(handle, &out);
    if(hret != HG_SUCCESS) {
        GAOFS_DATA->spdlogger()->error(
                "{}() Failed to respond",
                __func__);
    }
    // 释放rpc资源
    margo_free_input(handle, &in);
    margo_destroy(handle);
    return  HG_SUCCESS;
}

// 更新metadata条目
hg_return_t rpc_srv_update_metadentry(hg_handle_t handle) {
    // 目前好像还没有被调用
    // 初始化输入输出条目
    rpc_update_metadentry_in_t in{};
    rpc_err_out_t out{};
    auto ret = margo_get_input(handle, &in);
    if(ret != HG_SUCCESS) {
        GAOFS_DATA->spdlogger()->error(
                "{}() Failed to retrieve input from handle",
                __func__);
    }
    assert(ret == HG_SUCCESS);
    GAOFS_DATA->spdlogger()->debug(
            "{}() Got update metadentry RPC with path '{}'",
            __func__, in.path);

    // 实际操作
    try {
        gaofs::metadata::Metadata md = gaofs::metadata::get(in.path);
        if(in.block_flag == HG_TRUE)
            md.blocks(in.blocks);
        if(in.nlink_flag == HG_TRUE)
            md.link_count(in.nlink);
        if(in.size_flag == HG_TRUE)
            md.size(in.size);
        if(in.atime_flag == HG_TRUE)
            md.atime(in.atime);
        if(in.mtime_flag == HG_TRUE)
            md.mtime(in.mtime);
        if(in.ctime_flag == HG_TRUE)
            md.ctime(in.ctime);
        gaofs::metadata::update(in.path, md);
        out.err = 0;
    } catch (const std::exception& e) {
        GAOFS_DATA->spdlogger()->error(
                "{}() Failed to update entry",
                __func__);
        out.err = 1;
    }

    GAOFS_DATA->spdlogger()->debug("{}() Sending output '{}'",
                                   __func__, out.err);
    // 传输并且释放资源
    auto hret = margo_respond(handle, &out);
    if(hret != HG_SUCCESS) {
        GAOFS_DATA->spdlogger()->error(
                "{}() Failed to respond",
                __func__);
    }

    margo_free_input(handle, &in);
    margo_destroy(handle);
    return HG_SUCCESS;
}

hg_return_t rpc_srv_update_metadentry_size(hg_handle_t handle) {
    // 初始化输入输出结构
    rpc_update_metadentry_size_in_t in{};
    rpc_update_metadentry_size_out_t out{};
    auto ret = margo_get_input(handle, &in);
    if(ret != HG_SUCCESS) {
        GAOFS_DATA->spdlogger()->error(
                "{}() Failed to retrieve input from handle", __func__);
    }
    assert(ret == HG_SUCCESS);
    GAOFS_DATA->spdlogger()->debug(
            "{}() path: '{}', size: '{}', offset: '{}', append: '{}'",
            __func__, in.path, in.size, in.offset, in.append);

    // 进行具体操作
    try {
        gaofs::metadata::update_size(in.path, in.size, in.offset, (in.append == HG_TRUE));
        out.err = 0;
        // TODO :这里感觉有点问题，不一定是in.offset + in.size
        out.ret_size = in.offset + in.size;
    } catch (const gaofs::db_exception::NotFoundException& e) {
        out.err = ENOENT;
        GAOFS_DATA->spdlogger()->debug(
                "{}() Entry not found: '{}'",
                __func__, in.path);
    } catch (const std::exception& e) {
        out.err = EBUSY;
        GAOFS_DATA->spdlogger()->error(
                "{}() Failed to update metadentry size on DB: '{}'",
                __func__, e.what());
    }

    // 传输输出
    GAOFS_DATA->spdlogger()->debug(
            "{}() Sending output '{}'",
            __func__, out.err);
    auto hret = margo_respond(handle, &out);
    if(hret != HG_SUCCESS) {
        GAOFS_DATA->spdlogger()->error(
                "{}() Failed to respond",
                __func__);
    }

    // 释放资源
    margo_free_input(handle, &in);
    margo_destroy(handle);
    return HG_SUCCESS;
}

hg_return_t rpc_srv_get_metadentry_size(hg_handle_t handle) {
    // 初始化输入输出结构
    rpc_path_only_in_t in{};
    rpc_get_metadentry_size_out_t out{};
    auto ret = margo_get_input(handle, &in);
    if(ret != HG_SUCCESS) {
        GAOFS_DATA->spdlogger()->error(
                "{}() Failed to retrieve input from handle",
                __func__);
    }
    assert(ret == HG_SUCCESS);
    GAOFS_DATA->spdlogger()->debug(
            "{}() Got update metadentry size RPC with path '{}'",
            __func__, in.path);

    // 实际操作
    try {
        out.ret_size = gaofs::metadata::get_size(in.path);
        out.err = 0;
    } catch (const gaofs::db_exception::NotFoundException& e) {
        out.err = ENOENT;
        GAOFS_DATA->spdlogger()->debug(
                "{}() Entry not found: '{}'",
                __func__, in.path);
    } catch (const std::exception& e) {
        GAOFS_DATA->spdlogger()->error(
                "{}() Failed to get metadentry size from DB: '{}'",
                __func__, e.what());
        out.err = EBUSY;
    }

    GAOFS_DATA->spdlogger()->debug("{}() Sending output '{}'", __func__, out.err);
    // 传输输出
    auto hret = margo_respond(handle, &out);
    if(hret != HG_SUCCESS) {
        GAOFS_DATA->spdlogger()->error("{}() Failed to respond", __func__);
    }

    // 释放资源
    margo_free_input(handle, &in);
    margo_destroy(handle);
    return HG_SUCCESS;
}

// 与上面的函数处理方式不同，这个涉及对bulk的操作
hg_return_t rpc_srv_get_dirents(hg_handle_t handle) {
    //初始化输入输出参数
    rpc_get_dirents_in_t in{};
    rpc_get_dirents_out_t out{};
    out.err = EIO;
    out.dirents_size = 0;
    hg_bulk_t  bulk_handle = nullptr;
    auto ret = margo_get_input(handle, &in);
    if(ret != HG_SUCCESS) {
        GAOFS_DATA->spdlogger()->error(
                "{}() Could not get RPC input data with err '{}'",
                __func__, ret);
        out.err = EBUSY;
        return gaofs::rpc::cleanup_respond(&handle, &in, &out);
    }


    auto hgi = margo_get_info(handle);
    auto mid = margo_hg_info_get_instance(hgi);
    // 检索源缓冲区的大小
    auto bulk_size = margo_bulk_get_size(in.bulk_handle);
    GAOFS_DATA->spdlogger()->debug("{}() Got RPC: path '{}' bulk_size '{}'", __func__, in.path, bulk_size);

    // 具体操作，获取条目
    vector<pair<string, bool>> entries{};
    try {
        entries = gaofs::metadata::get_dirents(in.path);
    } catch(const std::exception& e) {
        GAOFS_DATA->spdlogger()->error("{}() Error during get_dirents(): '{}'",
                                       __func__, e.what());
        return gaofs::rpc::cleanup_respond(&handle, &in, &out);
    }

    GAOFS_DATA->spdlogger()->trace(
            "{}() path '{}' Read database with '{}' entries",
            __func__, in.path, entries.size());

    // 如果是个空目录，就直接返回
    if(entries.empty()) {
        out.err = 0;
        return gaofs::rpc::cleanup_respond(&handle, &in, &out);
    }
    // 计算目录名的总大小， 可以进行优化， 在db层就直接算好
    size_t  tot_names_size = 0;
    for(auto const& e :entries) {
        tot_names_size += e.first.size();
    }

    // 计算需要发送的缓冲区的大小, 这个char是'/0'
    size_t out_size = tot_names_size + entries.size() * (sizeof(bool) + sizeof(char));
    if(bulk_size < out_size) {
        GAOFS_DATA->spdlogger()->error(
                "{}() Entries do not fit source buffer. bulk_size '{}' < out_size '{}' must be satisfied!",
                __func__, bulk_size, out_size);
        out.err = ENOBUFS;
        return gaofs::rpc::cleanup_respond(&handle, &in, &out);
    }

    // 申请传递的缓冲区
    void* bulk_buf;
    // 创建bulk handle，并且分配内存, 但此时还不将bulk_buf指向这一个分配的内存
    ret = margo_bulk_create(mid, 1, nullptr, &out_size, HG_BULK_READ_ONLY, &bulk_handle);
    if(ret != HG_SUCCESS) {
        GAOFS_DATA->spdlogger()->error(
                "{}() Failed to create bulk handle",
                __func__);
        return gaofs::rpc::cleanup_respond(&handle, &in, &out, &bulk_handle);
    }

    // 访问内部分配的内存缓冲区并将其放入bulk_buf中
    uint32_t actual_count; // 段的数量。我们在这里使用一个，因为我们一次推入整个缓冲区
    ret = margo_bulk_access(bulk_handle, 0, out_size, HG_BULK_READ_ONLY, 1, &bulk_buf,
                            &out_size, &actual_count);
    if(ret != HG_SUCCESS || actual_count != 1) {
        GAOFS_DATA->spdlogger()->error(
                "{}() Failed to access allocated buffer from bulk handle",
                __func__);
        return gaofs::rpc::cleanup_respond(&handle, &in, &out, &bulk_handle);
    }

    GAOFS_DATA->spdlogger()->trace(
            "{}() path '{}' entries '{}' out_size '{}'. Set up local read only bulk handle and allocated buffer with size '{}'",
            __func__, in.path, entries.size(), out_size, out_size);

    // entries放进缓冲区里
    // 格式: bool name
    auto out_buff_ptr = static_cast<char*>(bulk_buf);
    auto bool_ptr = reinterpret_cast<bool*>(out_buff_ptr);
    auto name_ptr = out_buff_ptr + entries.size();

    for(auto const& e : entries) {
        if(e.first.empty()) {
            // 如果为空就说明出错了
            GAOFS_DATA->spdlogger()->warn(
                    "{}() Entry in readdir() empty. If this shows up, something else is very wrong.",
                    __func__);
        }
        *bool_ptr = e.second;
        bool_ptr++;
        const auto name = e.first.c_str();
        std::strcpy(name_ptr, name);
        // 有一个\0的位置
        name_ptr += e.first.size() + 1;
    }

    GAOFS_DATA->spdlogger()->trace(
            "{}() path '{}' entries '{}' out_size '{}'. Copied data to bulk_buffer. NEXT bulk_transfer",
            __func__, in.path, entries.size(), out_size);
    // 通过handle传递缓冲区， in_handle 和 本地的bulk_handle
    ret = margo_bulk_transfer(mid, HG_BULK_PUSH, hgi->addr, in.bulk_handle, 0,
                                bulk_handle, 0, out_size);
    if(ret != HG_SUCCESS) {
        GAOFS_DATA->spdlogger()->error(
                "{}() Failed to push '{}' dirents on path '{}' to client with bulk size '{}' and out_size '{}'",
                __func__, entries.size(), in.path, bulk_size,out_size);
        out.err = EBUSY;
        return gaofs::rpc::cleanup_respond(&handle, &in, &out, &bulk_handle);
    }

    out.dirents_size = entries.size();
    out.err = 0;
    // 传递并且释放资源
    GAOFS_DATA->spdlogger()->debug(
            "{}() Sending output response err '{}' dirents_size '{}'. DONE",
            __func__, out.err, out.dirents_size);
    // statistics
    if(GAOFS_DATA->enable_stats()) {
        GAOFS_DATA->stats()->add_value_iops(
                gaofs::utils::Stats::IopsOp::iops_dirent);
    }
    return gaofs::rpc::cleanup_respond(&handle, &in, &out, &bulk_handle);
}

// 和上面那个差不多
hg_return_t rpc_srv_get_dirents_extended(hg_handle_t handle) {
    //初始化输入输出参数
    rpc_get_dirents_in_t in{};
    rpc_get_dirents_out_t out{};
    out.err = EIO;
    out.dirents_size = 0;
    hg_bulk_t  bulk_handle = nullptr;
    auto ret = margo_get_input(handle, &in);
    if(ret != HG_SUCCESS) {
        GAOFS_DATA->spdlogger()->error(
                "{}() Could not get RPC input data with err '{}'",
                __func__, ret);
        out.err = EBUSY;
        return gaofs::rpc::cleanup_respond(&handle, &in, &out);
    }

    auto hgi = margo_get_info(handle);
    auto mid = margo_hg_info_get_instance(hgi);
    // 检索源缓冲区的大小
    auto bulk_size = margo_bulk_get_size(in.bulk_handle);
    GAOFS_DATA->spdlogger()->debug("{}() Got RPC: path '{}' bulk_size '{}'", __func__, in.path, bulk_size);

    // 具体操作，获取条目
    vector<tuple<string, bool, size_t, time_t>> entries{};
    try {
        entries = gaofs::metadata::get_dirents_extended(in.path);
    } catch(const std::exception& e) {
        GAOFS_DATA->spdlogger()->error("{}() Error during get_dirents(): '{}'",
                                       __func__, e.what());
        return gaofs::rpc::cleanup_respond(&handle, &in, &out);
    }

    GAOFS_DATA->spdlogger()->trace(
            "{}() path '{}' Read database with '{}' entries",
            __func__, in.path, entries.size());

    // 如果是个空目录，就直接返回
    if(entries.empty()) {
        out.err = 0;
        return gaofs::rpc::cleanup_respond(&handle, &in, &out);
    }
    // 计算目录名的总大小， 可以进行优化， 在db层就直接算好
    size_t  tot_names_size = 0;
    for(auto const& e :entries) {
        tot_names_size += (get<0>(e)).size();
    }

    // 计算需要发送的缓冲区的大小, 这个char是'/0'
    size_t out_size = tot_names_size + entries.size() * (sizeof(bool) + sizeof(char) + sizeof(size_t) + sizeof(time_t));
    if(bulk_size < out_size) {
        GAOFS_DATA->spdlogger()->error(
                "{}() Entries do not fit source buffer. bulk_size '{}' < out_size '{}' must be satisfied!",
                __func__, bulk_size, out_size);
        out.err = ENOBUFS;
        return gaofs::rpc::cleanup_respond(&handle, &in, &out);
    }

    // 申请传递的缓冲区
    void* bulk_buf;
    // 创建bulk handle，并且分配内存, 但此时还不将bulk_buf指向这一个分配的内存
    ret = margo_bulk_create(mid, 1, nullptr, &out_size, HG_BULK_READ_ONLY, &bulk_handle);
    if(ret != HG_SUCCESS) {
        GAOFS_DATA->spdlogger()->error(
                "{}() Failed to create bulk handle",
                __func__);
        return gaofs::rpc::cleanup_respond(&handle, &in, &out, &bulk_handle);
    }

    // 访问内部分配的内存缓冲区并将其放入bulk_buf中
    uint32_t actual_count; // 段的数量。我们在这里使用一个，因为我们一次推入整个缓冲区
    ret = margo_bulk_access(bulk_handle, 0, out_size, HG_BULK_READ_ONLY, 1, &bulk_buf,
                            &out_size, &actual_count);
    if(ret != HG_SUCCESS || actual_count != 1) {
        GAOFS_DATA->spdlogger()->error(
                "{}() Failed to access allocated buffer from bulk handle",
                __func__);
        return gaofs::rpc::cleanup_respond(&handle, &in, &out, &bulk_handle);
    }

    GAOFS_DATA->spdlogger()->trace(
            "{}() path '{}' entries '{}' out_size '{}'. Set up local read only bulk handle and allocated buffer with size '{}'",
            __func__, in.path, entries.size(), out_size, out_size);

    // entries放进缓冲区里
    // 格式: bool name
    auto out_buff_ptr = static_cast<char*>(bulk_buf);
    auto bool_ptr = reinterpret_cast<bool*>(out_buff_ptr);
    auto size_ptr = reinterpret_cast<size_t*>((out_buff_ptr) + (entries.size() * sizeof(bool)));
    auto ctime_ptr = reinterpret_cast<time_t*>((out_buff_ptr) + (entries.size() * (sizeof(bool) + sizeof(size_t))));
    auto name_ptr = out_buff_ptr + (entries.size() * (sizeof(bool) + sizeof(size_t) + sizeof(time_t)));

    for(auto const& e : entries) {
        if((get<0>(e)).empty()) {
            // 如果为空就说明出错了
            GAOFS_DATA->spdlogger()->warn(
                    "{}() Entry in readdir() empty. If this shows up, something else is very wrong.",
                    __func__);
        }
        *bool_ptr = (get<1>(e));
        bool_ptr++;

        *size_ptr = (get<2>(e));
        size_ptr++;

        *ctime_ptr = (get<3>(e));
        ctime_ptr++;
        const auto name = (get<0>(e)).c_str();
        std::strcpy(name_ptr, name);
        // 有一个\0的位置
        name_ptr += ((get<0>(e)).size() + 1);
    }

    GAOFS_DATA->spdlogger()->trace(
            "{}() path '{}' entries '{}' out_size '{}'. Copied data to bulk_buffer. NEXT bulk_transfer",
            __func__, in.path, entries.size(), out_size);
    // 通过handle传递缓冲区， in_handle 和 本地的bulk_handle
    ret = margo_bulk_transfer(mid, HG_BULK_PUSH, hgi->addr, in.bulk_handle, 0,
                              bulk_handle, 0, out_size);
    if(ret != HG_SUCCESS) {
        GAOFS_DATA->spdlogger()->error(
                "{}() Failed to push '{}' dirents on path '{}' to client with bulk size '{}' and out_size '{}'",
                __func__, entries.size(), in.path, bulk_size,out_size);
        out.err = EBUSY;
        return gaofs::rpc::cleanup_respond(&handle, &in, &out, &bulk_handle);
    }

    out.dirents_size = entries.size();
    out.err = 0;
    // 传递并且释放资源
    GAOFS_DATA->spdlogger()->debug(
            "{}() Sending output response err '{}' dirents_size '{}'. DONE",
            __func__, out.err, out.dirents_size);
    return gaofs::rpc::cleanup_respond(&handle, &in, &out, &bulk_handle);
}

// SYMLINKS
#ifdef HAS_SYMLINKS
/*
 * 创建软连接
 */
hg_return_t
rpc_srv_mk_symlink(hg_handle_t handle) {
    rpc_mk_symlink_in_t in{};
    rpc_err_out_t out{};

    auto ret = margo_get_input(handle, &in);
    if(ret != HG_SUCCESS) {
        GAOFS_DATA->spdlogger()->error(
                "{}() Failed to retrieve input from handle", __func__);
    }
    GAOFS_DATA->spdlogger()->debug("{}() Got RPC with path '{}'", __func__,
                                  in.path);

    try {
        gaofs::metadata::Metadata md = {gaofs::metadata::LINK_MODE,
                                       in.target_path};
        // create metadentry
        gaofs::metadata::create(in.path, md);
        out.err = 0;
    } catch(const std::exception& e) {
        GAOFS_DATA->spdlogger()->error("{}() Failed to create metadentry: '{}'",
                                      __func__, e.what());
        out.err = -1;
    }
    GAOFS_DATA->spdlogger()->debug("{}() Sending output err '{}'", __func__,
                                  out.err);
    auto hret = margo_respond(handle, &out);
    if(hret != HG_SUCCESS) {
        GAOFS_DATA->spdlogger()->error("{}() Failed to respond", __func__);
    }

    // Destroy handle when finished
    margo_free_input(handle, &in);
    margo_destroy(handle);
    return HG_SUCCESS;
}

#endif

} // namespace

DEFINE_MARGO_RPC_HANDLER(rpc_srv_create)

DEFINE_MARGO_RPC_HANDLER(rpc_srv_stat)

DEFINE_MARGO_RPC_HANDLER(rpc_srv_decr_size)

DEFINE_MARGO_RPC_HANDLER(rpc_srv_remove_metadata)

DEFINE_MARGO_RPC_HANDLER(rpc_srv_remove_data)

DEFINE_MARGO_RPC_HANDLER(rpc_srv_update_metadentry)

DEFINE_MARGO_RPC_HANDLER(rpc_srv_update_metadentry_size)

DEFINE_MARGO_RPC_HANDLER(rpc_srv_get_metadentry_size)

DEFINE_MARGO_RPC_HANDLER(rpc_srv_get_dirents)

DEFINE_MARGO_RPC_HANDLER(rpc_srv_get_dirents_extended)

#ifdef HAS_SYMLINKS
DEFINE_MARGO_RPC_HANDLER(rpc_srv_mk_symlink)
#endif