//
// Created by DELL on 2023/2/27.
//

#include <daemon/util.hpp>
#include <daemon/daemon.hpp>

#include <common/rpc/rpc_util.hpp>
using namespace std;

namespace gaofs::utils {

void populate_hosts_file() {
    const auto& hosts_file = GAOFS_DATA->hosts_file();
    GAOFS_DATA->spdlogger()->debug(
            "{}() Populating hosts file: '{}'",
            __func__, hosts_file);
    // 如果没有文件，那么生成空文件；如果有文件，那么在文件尾追加
    ofstream lfstream(hosts_file, ios::out | ios::app);
    if(!lfstream) {
        throw runtime_error(fmt::format("Failed to open hosts file '{}': {}",
                                        hosts_file, strerror(errno)));
    }

    auto hostname = GAOFS_DATA->rootdir_suffix().empty()? gaofs::rpc::get_my_hostname(true):
                    fmt::format("{}#{}", gaofs::rpc::get_my_hostname(true), GAOFS_DATA->rootdir_suffix());

    // 写入hostfile
    lfstream << fmt::format("{} {}", hostname, RPC_DATA->self_addr_str()) << endl;
    if(!lfstream) {
        throw runtime_error(
                fmt::format("Failed to write on hosts file '{}': {}",
                            hosts_file, strerror(errno)));
    }

    lfstream.close();
}

void destroy_hosts_file() {
    remove(GAOFS_DATA->hosts_file().c_str());
}

} // namespace gaofs::utils
