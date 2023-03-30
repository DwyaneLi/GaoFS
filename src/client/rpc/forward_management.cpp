//
// Created by DELL on 2023/3/19.
//

#include <client/rpc/forward_management.hpp>
#include <client/logging.hpp>
#include <client/preload_util.hpp>
#include <client/rpc/rpc_types.hpp>

namespace gaofs::rpc {

// 从正在运行的守护进程获取fs配置信息，并将其传输到库的内存中

bool forward_get_fs_config() {

    auto endp = CTX->hosts().at(CTX->local_host_id());
    gaofs::rpc::fs_config::output out;

    try {
        LOG(DEBUG, "Retrieving file system configurations from daemon");
        out = ld_network_service->post<gaofs::rpc::fs_config>(endp).get().at(0);
    } catch(const std::exception& ex) {
        LOG(ERROR, "Retrieving fs configurations from daemon");
        return false;
    }

    CTX->mountdir(out.mountdir());
    LOG(INFO, "Mountdir: '{}'", CTX->mountdir());

    CTX->fs_conf()->rootdir = out.rootdir();
    CTX->fs_conf()->atime_state = out.atime_state();
    CTX->fs_conf()->mtime_state = out.mtime_state();
    CTX->fs_conf()->ctime_state = out.ctime_state();
    CTX->fs_conf()->link_cnt_state = out.link_cnt_state();
    CTX->fs_conf()->blocks_state = out.blocks_state();
    CTX->fs_conf()->uid = out.uid();
    CTX->fs_conf()->gid = out.gid();

    LOG(DEBUG, "Got response with mountdir {}", out.mountdir());

    return true;
}
} // namespace gaofs::rpc

