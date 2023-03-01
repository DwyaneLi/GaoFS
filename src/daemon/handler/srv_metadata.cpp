//
// Created by DELL on 2023/2/28.
//

#include <daemon/handler/rpc_util.hpp>
#include <daemon/handler/rpc_defs.hpp>
#include <daemon/ops/metadentry.hpp>
#include <daemon/backend/metadata/db.hpp> // exception
#include <common/rpc/rpc_types.hpp>

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

    // TODO: statistic

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
hg_return_t rpc_srv_remove_metadata(hg_handle_t handle) {
    // TODO:first chunk处理
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
        out.err = 0;
        out.mode = md.mode();
        out.size = md.size();
        if constexpr(gaofs::config::metadata::implicit_data_removal) {
            if(S_ISREG(md.mode()) && (md.size()) != 0) {
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
    // TODO:statistics

    return HG_SUCCESS;
}


// 处理程序只是发出删除本地文件系统上所有块文件的命令
// 如果有隐式删除的config，这个在之前在remove metadata里就执行了。
// 这个和metadata没关系
hg_return_t rpc_srv_remove_data(hg_handle_t handle) {
    // TODO: first chunk
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
        GAOFS_DATA->storage()->destroy_chunk_space(in.path);
        out.err = 0;
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

}