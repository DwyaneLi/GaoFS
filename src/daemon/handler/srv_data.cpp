//
// Created by DELL on 2023/3/6.
//
#include <daemon/ops/data.hpp>
#include <daemon/handler/rpc_defs.hpp>
#include <daemon/handler/rpc_util.hpp>

#include <common/rpc/rpc_types.hpp>

// TODO: agios

using namespace std;

namespace {

// 服务一个写请求，传输与这个守护进程相关的块，并将它们存储在节点本地FS上
// 1. 设置所有RPC相关信息
// 2.为批量传输缓冲区分配空间
// 3.通过处理RPC输入，根据客户端定义的间隔(此写操作的开始和结束块id)计算散列到此守护进程的块id。
// 客户端不向守护进程提供块id列表（只有告诉你一共有多少个块），因为它是动态数据（大小不确定），
// 不能作为RPC输入结构的一部分。因此，该信息也需要通过批量传输来获取，从而为整个写操作增加了不必要的延迟。
// 4. 等待所有微线程完成将每个任务报告的所有完整写入数据大小相加。
// 5. 响应客户端(当所有后端写操作完成时)并清理RPC资源。任何错误都将在RPC输出结构中报告。
// 注意，当任务遇到错误时，后台写操作不会在运行中取消。
hg_return_t rpc_srv_write(hg_handle_t handle) {
    /*
     * 1.设置所有RPC相关信息
     */
    // 初始化输入输出结构
    rpc_write_data_in_t in{};
    rpc_data_out_t out{};
    hg_bulk_t  bulk_handle = nullptr;
    out.err = EIO;
    out.io_size = 0;
    auto ret = margo_get_input(handle, &in);
    if(ret != HG_SUCCESS) {
        GAOFS_DATA->spdlogger()->error(
                "{}() Could not get RPC input data with err {}",
                __func__, ret);
        return gaofs::rpc::cleanup_respond(&handle, &in, &out, &bulk_handle);
    }

    auto hgi = margo_get_info(handle);
    auto mid = margo_hg_info_get_instance(hgi);
    auto bulk_size = margo_bulk_get_size(in.bulk_handle);
    GAOFS_DATA->spdlogger()->debug(
            "{}() path: '{}' chunk_start '{}' chunk_end '{}' chunk_n '{}' total_chunk_size '{}' bulk_size: '{}' offset: '{}'",
            __func__, in.path, in.chunk_start, in.chunk_end, in.chunk_n, in.total_chunk_size,
            bulk_size, in.offset);
    // TODO: AGIOS

    /*
     * 2.为批量传输缓冲区分配空间
     */
    void* bulk_buf; // 用于传送的缓冲区
    vector<char*> bulk_buf_ptrs(in.chunk_n);
    // 创建bulk handle
    ret = margo_bulk_create(mid, 1, nullptr, &in.total_chunk_size, HG_BULK_READWRITE, &bulk_handle);
    if(ret != HG_SUCCESS) {
        GAOFS_DATA->spdlogger()->error("{}() Failed to create bulk handle", __func__);
        // 为什么是nullptr? 因为没有创建成功所以没有bulkhandle需要释放
        return gaofs::rpc::cleanup_respond(&handle, &in, &out, static_cast<hg_bulk_t*>(nullptr));
    }
    // 访问内部分配的内存缓冲区并将其放入buf_ptrs中
    // 看了margo的，感觉没必要，直接放到上一步也许可行？
    uint32_t actual_count;
    ret = margo_bulk_access(bulk_handle, 0, in.total_chunk_size, HG_BULK_READWRITE,
                            1, &bulk_buf, &in.total_chunk_size, &actual_count);
    if(ret != HG_SUCCESS || actual_count != 1) {
        GAOFS_DATA->spdlogger()->error("{}() Failed to access allocated buffer from bulk handle",
                                       __func__);
        gaofs::rpc::cleanup_respond(&handle, &in, &out, &bulk_handle);
    }
    auto const host_id = in.host_id;
    auto const host_size = in.host_size;

    // 存在这个主机上的chunk_id
    vector<uint64_t> chnk_ids_host(in.chunk_n);
    // 在本主机上的chunk块数
    auto chnk_id_curr = static_cast<uint64_t>(0);
    // 每个chunk的size
    vector<uint64_t> chnk_sizes(in.chunk_n);
    // 还剩下多少大小可以分配用于写入的块
    auto chnk_size_left_host = in.total_chunk_size;
    // 在bulk_buf中的移动指针
    auto chnk_ptr = static_cast<char*>(bulk_buf);

    /*
     * 需要考虑以下几种情况：
     * 1. 第一个块是否在这个节点上，并且是否有偏移？
     * 2. 如果偏移，是否只有一个块要被写入？
     * 3. 如果没有偏移，是否只有一个块要被写入？
     * 4. 开始块和结束块之间的块的大小为CHUNKSIZE
     * 5. 最后一个块(如果写入多个块):不要写入CHUNKSIZE，而是为这个目标写入chnk_size_left。
     *    如果只写入一个块，最后一个块也可以发生。这被2和3覆盖了。
     */
    auto transer_size = (bulk_size <= gaofs::config::rpc::chunksize)? bulk_size : gaofs::config::rpc::chunksize;
    uint64_t origin_offset;
    uint64_t local_offset;
    gaofs::data::ChunkWriteOperation chunk_op{in.path, in.chunk_n};

    /*
     * 计算该主机上块对应的大小，传输数据，并启动写入磁盘的任务
     * */
    // 找到第一个散列到本机上的chunk
    for(auto chnk_id_file = in.chunk_start; chnk_id_file <= in.chunk_end && chnk_id_curr < in.chunk_n; chnk_id_file++) {
        // TODO: forwarding
        // 不是定位在本机上的直接跳过
        if(RPC_DATA->distributor()->locate_data(in.path, chnk_id_file, host_size) != host_id) {
            GAOFS_DATA->spdlogger()->trace("{}() chunkid '{}' ignored as it does not match to this host with id '{}'. chnk_id_curr '{}'",
                                           __func__, chnk_id_file, host_id, chnk_id_curr);
            continue;
        }

        chnk_ids_host[chnk_id_curr] = chnk_id_file; // 保存
        // 如果第一个chunk在这个主机，且有offset
        if(chnk_id_file == in.chunk_start && in.offset > 0) {
            size_t offset_transfer_size = 0;
            // 如果要写的很小的话，就只要写一个块，并且写的大小加上offset好不到一个chunk的大小
            if(in.offset + bulk_size <= gaofs::config::rpc::chunksize)
                offset_transfer_size = bulk_size;
            else
                offset_transfer_size = static_cast<size_t>(gaofs::config::rpc::chunksize - in.offset);
            // 进行缓冲区的传递
            ret = margo_bulk_transfer(mid, HG_BULK_PULL, hgi->addr,
                                      in.bulk_handle, 0, bulk_handle, 0, offset_transfer_size);
            if(ret != HG_SUCCESS) {
                GAOFS_DATA->spdlogger()->error("{}() Failed to pull data from client for chunk {} (startchunk {}; endchunk {})",
                                               __func__, chnk_id_file, in.chunk_start, in.chunk_end - 1);
                out.err = EBUSY;
                return gaofs::rpc::cleanup_respond(&handle, &in, &out, &bulk_handle);
            }
            // 存储相应的信息
            bulk_buf_ptrs[chnk_id_curr] = chnk_ptr;
            chnk_sizes[chnk_id_curr] = offset_transfer_size;
            chnk_ptr += offset_transfer_size;
            chnk_size_left_host -= offset_transfer_size;
        } else {
            local_offset = in.total_chunk_size - chnk_size_left_host;
            // 块的起始偏移量依赖于写操作中的给定偏移量
            // 计算origin offset
            if(in.offset > 0)
                origin_offset = (gaofs::config::rpc::chunksize - in.offset) +
                        ((chnk_id_file - in.chunk_start) - 1) * gaofs::config::rpc::chunksize;
            else
                origin_offset = (chnk_id_file - in.chunk_start) * gaofs::config::rpc::chunksize;
            // 最后一个块可能有不同的transfer_size
            if(chnk_id_curr == in.chunk_n - 1)
                transer_size = chnk_size_left_host;
            GAOFS_DATA->spdlogger()->trace(
                    "{}() BULK_TRANSFER_PULL hostid {} file {} chnkid {} total_size {} size_left {} origin offset {} local offset {} transfersize {}",
                    __func__, host_id, in.path, chnk_id_file,
                    in.total_chunk_size, chnk_size_left_host, origin_offset,
                    local_offset, transer_size);
            // 通过RDMA把源数据放到本地的缓冲区
            ret = margo_bulk_transfer(mid, HG_BULK_PULL, hgi->addr,
                                      in.bulk_handle, origin_offset, bulk_handle,
                                      local_offset, transer_size);
            if(ret != HG_SUCCESS) {
                GAOFS_DATA->spdlogger()->error(
                        "{}() Failed to pull data from client. file {} chunk {} (startchunk {}; endchunk {})",
                        __func__, in.path, chnk_id_file, in.chunk_start, in.chunk_end - 1);
            }
            // 进行信息的纪录
            bulk_buf_ptrs[chnk_id_curr] = chnk_ptr;
            chnk_sizes[chnk_id_curr] = transer_size;
            chnk_ptr += transer_size;
            chnk_size_left_host -= transer_size;
        }
        try {
            // 启动微线程
            chunk_op.write_nonblock(chnk_id_curr, chnk_id_file, bulk_buf_ptrs[chnk_id_curr],
                                    chnk_sizes[chnk_id_curr], (chnk_id_file == in.chunk_start)?in.offset:0);
        } catch (const gaofs::data::ChunkWriteOpException& e) {
            GAOFS_DATA->spdlogger()->error("{}() while write_nonblock err '{}'",
                                           __func__, e.what());
            return gaofs::rpc::cleanup_respond(&handle, &in, &out, &bulk_handle);
        }
        chnk_id_curr++;
    }

    if(chnk_size_left_host != 0) {
        GAOFS_DATA->spdlogger()->warn(
                "{}() Not all chunks were detected!!! Size left {}",
                __func__, chnk_size_left_host);
    }

    /*
     * 4. 读取任务结果(wait_for_tasks())并累积在out.io_size中
     */
    auto write_result = chunk_op.wait_for_tasks();
    out.err = write_result.first;
    out.io_size = write_result.second;

    if(in.total_chunk_size != out.io_size) {
        GAOFS_DATA->spdlogger()->warn("{}() total chunk size {} and out.io_size {} mismatch!",
                                      __func__, in.total_chunk_size, out.io_size);
    }

    /*
     * 5. 传输输出结果并且清理资源
     * */
    GAOFS_DATA->spdlogger()->debug("{}() Sending output response {}", __func__, out.err);
    auto hret = gaofs::rpc::cleanup_respond(&handle, &in, &out, &bulk_handle);
    // TODO Statistic
    return hret;
}


// 读文件，与上面的类似
hg_return_t rpc_srv_read(hg_handle_t handle) {
    /*
     * 设置和初始化
     */
    rpc_read_data_in_t in{};
    rpc_data_out_t out{};
    hg_bulk_t bulk_handle = nullptr;
    // 初始化输出
    out.err = EIO;
    out.io_size = 0;
    auto ret = margo_get_input(handle, &in);
    if(ret != HG_SUCCESS) {
        GAOFS_DATA->spdlogger()->error("{}() Could not get RPC input data with err {}",
                                       __func__, ret);
        return gaofs::rpc::cleanup_respond(&handle, &in, &out, &bulk_handle);
    }
    auto hgi = margo_get_info(handle);
    auto mid = margo_hg_info_get_instance(hgi);
    auto bulk_size = margo_bulk_get_size(in.bulk_handle);

    GAOFS_DATA->spdlogger()->debug(
            "{}() path: '{}' chunk_start '{}' chunk_end '{}' chunk_n '{}' total_chunk_size '{}' bulk_size: '{}' offset: '{}'",
            __func__, in.path, in.chunk_start, in.chunk_end, in.chunk_n, in.total_chunk_size, bulk_size, in.offset);

    // TODO: AGIOS

    /*
     * 2.为批量传输缓冲区分配空间
     */
    void* bulk_buf;
    vector<char*> bulk_buf_ptrs(in.chunk_n);
    // 创造一个bulk_handle， 并且分配缓冲区
    ret = margo_bulk_create(mid, 1, nullptr, &in.total_chunk_size, HG_BULK_READWRITE, &bulk_handle);
    if(ret != HG_SUCCESS) {
        GAOFS_DATA->spdlogger()->error("{}() Failed to create bulk handle", __func__);
        return gaofs::rpc::cleanup_respond(&handle, &in, &out, static_cast<hg_bulk_t*>(nullptr));
    }
    // 将bulk的访问交给bulk_handle
    uint32_t actual_count;
    ret = margo_bulk_access(bulk_handle, 0, in.total_chunk_size, HG_BULK_READWRITE,
                            1, &bulk_buf, &in.total_chunk_size, &actual_count);
    if(ret != HG_SUCCESS || actual_count != 1) {
        GAOFS_DATA->spdlogger()->error("{}() Failed to access allocated buffer from bulk handle",
                                       __func__);
        return  gaofs::rpc::cleanup_respond(&handle, &in, &out, &bulk_handle);
    }
    // TODO: FORWARDING
    auto const host_id = in.host_id;
    auto const host_size = in.host_size;
    // 各种临时变量的初始化
    // 本机上保存的块
    vector<uint64_t> chnk_ids_host(in.chunk_n);
    // 数组索引
    auto chnk_id_curr = static_cast<uint64_t>(0);
    // 块的大小
    vector<uint64_t> chnk_sizes(in.chunk_n);
    // 每个块在各自缓冲区的偏移
    vector<uint64_t> local_offsets(in.chunk_n);
    vector<uint64_t> origin_offsets(in.chunk_n);
    // 还剩多少数据
    auto chnk_size_left_host = in.total_chunk_size;
    // 缓冲区移动指针
    auto chnk_ptr = static_cast<char*>(bulk_buf);
    // 传递大小
    auto transfer_size = (bulk_size <= gaofs::config::rpc::chunksize) ? bulk_size : gaofs::config::rpc::chunksize;
    // 初始化用于执行任务的类
    gaofs::data::ChunkReadOperation chunk_read_op{in.path, in.chunk_n};

    /*
     * 3. 从文件系统中读文件内容到内存
     */
    for(auto chnk_id_file = in.chunk_start; chnk_id_file <= in.chunk_end && chnk_id_curr < in.chunk_n; chnk_id_file++) {
        // 不在本机上的就直接跳过
        if(RPC_DATA->distributor()->locate_data(in.path, chnk_id_file, in.host_size) != host_id) {
            GAOFS_DATA->spdlogger()->trace("{}() chunkid '{}' ignored as it does not match to this host with id '{}'. chnk_id_curr '{}'",
                                           __func__, chnk_id_file, host_id, chnk_id_curr);
            continue;
        }
        // TODO： statistics

        // 保存到数组中
        chnk_ids_host[chnk_id_curr] = chnk_id_file;
        if(chnk_id_file == in.chunk_start && in.offset > 0) {
            size_t offset_transfer_size = 0;
            if(in.offset + bulk_size <= gaofs::config::rpc::chunksize)
                offset_transfer_size = bulk_size;
            else
                offset_transfer_size = static_cast<size_t>(gaofs::config::rpc::chunksize - in.offset);
            // 记录相关信息
            local_offsets[chnk_id_curr] = 0;
            origin_offsets[chnk_id_curr] = 0;
            bulk_buf_ptrs[chnk_id_curr] = chnk_ptr;
            chnk_sizes[chnk_id_curr] = offset_transfer_size;
            chnk_ptr += offset_transfer_size;
            chnk_size_left_host -= offset_transfer_size;
        } else {
            local_offsets[chnk_id_curr] = in.total_chunk_size - chnk_size_left_host;
            if(in.offset > 0) {
                origin_offsets[chnk_id_curr] = gaofs::config::rpc::chunksize - in.offset +
                        (chnk_id_file - in.chunk_start - 1) * gaofs::config::rpc::chunksize;
            } else {
                 origin_offsets[chnk_id_curr] = (chnk_id_file - in.chunk_start) * gaofs::config::rpc::chunksize;
            }
            if(chnk_id_curr == in.chunk_n - 1)
                transfer_size = chnk_size_left_host;
            bulk_buf_ptrs[chnk_id_curr] = chnk_ptr;
            chnk_sizes[chnk_id_curr] = transfer_size;
            chnk_ptr += transfer_size;
            chnk_size_left_host -= transfer_size;
        }

        try {
            // 进行将文件读取到内存然后推过去的行为
            chunk_read_op.read_nonblock(chnk_id_curr, chnk_ids_host[chnk_id_curr], bulk_buf_ptrs[chnk_id_curr],
                                        chnk_sizes[chnk_id_curr], (chnk_id_file == in.chunk_start)?in.offset:0);
        } catch (const gaofs::data::ChunkReadOpException& e) {
            GAOFS_DATA->spdlogger()->error("{}() while read_nonblock err '{}'",
                                           __func__, e.what());
            return gaofs::rpc::cleanup_respond(&handle, &in, &out, &bulk_handle);
        }
        chnk_id_curr++;
    }

    /*
     * 4.等待输出结果
     */
    // 构造用于bulk wait的参数
    gaofs::data::ChunkReadOperation::bulk_args bulk_args{};
    bulk_args.mid = mid;
    bulk_args.local_offsets = &local_offsets;
    bulk_args.local_bulk_handle = bulk_handle;
    bulk_args.origin_offsets = &origin_offsets;
    bulk_args.origin_bulk_handle = in.bulk_handle;
    bulk_args.origin_addr = hgi->addr;
    bulk_args.chunk_ids = &chnk_ids_host;
    // wait
    auto read_result = chunk_read_op.wait_for_tasks_and_push_back(bulk_args);
    out.err = read_result.first;
    out.io_size = read_result.second;

    /*
     * 5. 传输结果并且释放资源
     */
    GAOFS_DATA->spdlogger()->debug("{}() Sending output response, err: {}",
                                   __func__, out.err);
    auto hret = gaofs::rpc::cleanup_respond(&handle, &in, &out, &bulk_handle);
    // TODO:statistics
    return hret;
}

// 为文件截断请求提供服务，并删除这个守护进程上所有对应的块文件。
// 截断操作包括减少元数据项的文件大小(如果元数据被哈希到这里)，并删除超过新文件大小的所有相应块。
hg_return_t rpc_srv_truncate(hg_handle_t handle) {
    // 初始化输入输出参数
    rpc_trunc_data_in_t in{};
    rpc_err_out_t out{};
    out.err = EIO;
    // 获取输入结构
    auto ret = margo_get_input(handle, &in);
    if(ret != HG_SUCCESS) {
        GAOFS_DATA->spdlogger()->error(
                "{}() Could not get RPC input data with err {}",
                __func__);
    }
    GAOFS_DATA->spdlogger()->debug("{}() path: '{}', length: '{}'",
                                   __func__, in.path, in.length);

    gaofs::data::ChunkTruncateOperation chunk_op{in.path};
    // 实际操作
    try {
        chunk_op.truncate(in.length, in.host_id, in.host_size);
    } catch (const gaofs::data::ChunkMetaOpException& e) {
        GAOFS_DATA->spdlogger()->error("{}() while truncate err '{}'",
                                       __func__, e.what());
        return gaofs::rpc::cleanup_respond(&handle, &in, &out);
    }

    // 等待并且输出
    out.err = chunk_op.wait_for_task();
    GAOFS_DATA->spdlogger()->debug("{}() Sending output response '{}'", __func__,
                                   out.err);
    return gaofs::rpc::cleanup_respond(&handle, &in, &out);
}

// 获取chunk状态
hg_return_t rpc_srv_get_chunk_stat(hg_handle_t handle) {
    GAOFS_DATA->spdlogger()->debug("{}() enter", __func__);
    // 初始化输出结构
    rpc_chunk_stat_out_t out{};
    out.err = EIO;
    // 直接从storage中获取就行了
    try {
        auto chnk_stat = GAOFS_DATA->storage()->chunk_stat();
        out.chunk_total = chnk_stat.chunk_total;
        out.chunk_free = chnk_stat.chunk_free;
        out.chunk_size = chnk_stat.chunk_size;
        out.err = 0;
    } catch (const gaofs::data::ChunkStorageException& e) {
        GAOFS_DATA->spdlogger()->error("{}() {}", __func__, e.what());
        out.err = e.code().value();
    } catch (const std::exception& e) {
        GAOFS_DATA->spdlogger()->error("{}() Unexpected error when chunk stat '{}'",
                                       __func__, e.what());
        out.err = EAGAIN;
    }

    return gaofs::rpc::cleanup_respond(&handle, &out);
}

} // namespace
