//
// Created by DELL on 2023/3/6.
//

#include <daemon/daemon.hpp>
#include <daemon/handler/rpc_defs.hpp>
#include <common/rpc/rpc_types.hpp>

extern "C" {
#include <unistd.h>
}

using namespace std;

namespace {

// 获取文件系统的信息
// 最值得注意的是，这是客户端获取GekkoFS可访问路径信息的地方。
hg_return_t rpc_srv_get_fs_config(hg_handle_t handle) {
    // 初始化输入输出函数
    rpc_config_out_t out{};

    GAOFS_DATA->spdlogger()->debug("{}() Got config RPC", __func__);

    // get fs config
    out.mountdir = GAOFS_DATA->mountdir().c_str();
    out.rootdir = GAOFS_DATA->rootdir().c_str();
    out.mtime_state = static_cast<hg_bool_t>(GAOFS_DATA->mtime_state());
    out.ctime_state = static_cast<hg_bool_t>(GAOFS_DATA->ctime_state());
    out.atime_state = static_cast<hg_bool_t>(GAOFS_DATA->atime_state());
    out.link_cnt_state = static_cast<hg_bool_t>(GAOFS_DATA->link_cnt_state());
    out.blocks_state = static_cast<hg_bool_t>(GAOFS_DATA->blocks_state());
    out.gid = getuid();
    out.gid = getgid();
    GAOFS_DATA->spdlogger()->debug("{}() Sending output configs back to library", __func__);

    auto hret = margo_respond(handle, &out);
    if(hret == HG_SUCCESS) {
        GAOFS_DATA->spdlogger()->error("{}() Failed to respond to client to serve file system configurations",
                                       __func__);
    }

    // 释放资源
    margo_destroy(handle);
    return HG_SUCCESS;
}

} // namespcace

DEFINE_MARGO_RPC_HANDLER(rpc_srv_get_fs_config)

