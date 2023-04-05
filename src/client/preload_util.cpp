//
// Created by DELL on 2023/3/19.
//

#include <client/preload_util.hpp>
#include <client/env.hpp>
#include <client/logging.hpp>
#include <client/rpc/forward_metadata.hpp>

#include <common/rpc/distributor.hpp>
#include <common/rpc/rpc_util.hpp>
#include <common/env_util.hpp>
#include <common/common_defs.hpp>

#include <hermes.hpp>

#include <fstream>
#include <sstream>
#include <regex>
#include <csignal>
#include <random>

extern "C" {
#include <sys/sysmacros.h>
}

using namespace std;

namespace  {

// 通过uri查询主机端点
hermes::endpoint lookup_endpoint(const string& uri, std::size_t max_retries = 3) {
    LOG(DEBUG, "Looking up address \"{}\"", uri);
    std::random_device rd;
    std::size_t attempts = 0;
    std::string error_msg;

    // 尝试3次（最多）
    do {
        try {
            return ld_network_service->lookup(uri);
        } catch(const exception& ex) {
            error_msg = ex.what();

            LOG(WARNING, "Failed to lookup address '{}'. Attempts [{}/{}]", uri,
                attempts + 1, max_retries);

            // Wait a random amount of time and try again
            std::mt19937 g(rd()); // seed the random generator
            std::uniform_int_distribution<> distr(
                    50, 50 * (attempts + 2)); // define the range
            std::this_thread::sleep_for(std::chrono::milliseconds(distr(g)));
            continue;
        }
    } while(++attempts < max_retries);

    throw std::runtime_error(
            fmt::format("Endpoint for address '{}' could not be found ({})",
                        uri, error_msg));
}

// 从RPC server给出的uri中提取协议
void extract_protocol(const string& uri) {
    if(uri.rfind("://") == string::npos) {
        // invalid format. kill client
        throw runtime_error(fmt::format("Invalid format for URI: '{}'", uri));
    }
    string protocol{};
    // 判断共享内存协议
    if(uri.find(gaofs::rpc::protocol::ofi_sockets) != string::npos) {
        protocol = gaofs::rpc::protocol::ofi_sockets;
    } else if(uri.find(gaofs::rpc::protocol::ofi_psm2) != string::npos) {
        protocol = gaofs::rpc::protocol::ofi_psm2;
    } else if(uri.find(gaofs::rpc::protocol::ofi_verbs) != string::npos) {
        protocol = gaofs::rpc::protocol::ofi_verbs;
    }
    // check for shared memory protocol. Can be plain shared memory or real ofi
    // protocol + auto_sm
    // 判断是否使用na_sm
    if(uri.find(gaofs::rpc::protocol::na_sm) != string::npos) {
        if(protocol.empty())
            protocol = gaofs::rpc::protocol::na_sm;
        else
            CTX->auto_sm(true);
    }
    if(protocol.empty()) {
        // unsupported protocol. kill client
        throw runtime_error(fmt::format(
                "Unsupported RPC protocol found in hosts file with URI: '{}'",
                uri));
    }
    LOG(INFO,
        "RPC protocol '{}' extracted from hosts file. Using auto_sm is '{}'",
        protocol, CTX->auto_sm());
    CTX->rpc_protocol(protocol);
}

// 根据指定路径读取守护进程生成器hosts文件，返回主机和URI地址
vector<pair<string, string>> load_hostfile(const std::string& path) {
    LOG(DEBUG, "Loading hosts file: \"{}\"", path);
    // 转化为流
    ifstream lf(path);
    if(!lf) {
        throw runtime_error(fmt::format("Failed to open hosts file '{}': {}",
                                        path, strerror(errno)));
    }
    vector<pair<string, string>> hosts;
    const regex line_re("^(\\S+)\\s+(\\S+)$",
                        regex::ECMAScript | regex::optimize);
    string line;
    string host;
    string uri;
    std::smatch match;
    while(getline(lf, line)) {
        if(!regex_match(line, match, line_re)) {

            LOG(ERROR, "Unrecognized line format: [path: '{}', line: '{}']",
                path, line);

            throw runtime_error(
                    fmt::format("unrecognized line format: '{}'", line));
        }
        host = match[1];
        uri = match[2];
        hosts.emplace_back(host, uri);
    }
    if(hosts.empty()) {
        throw runtime_error(
                "Hosts file found but no suitable addresses could be extracted");
    }
    // 为啥只对第一个host提取协议？是因为是本机吗？
    extract_protocol(hosts[0].second);
    // 对主机进行排序，以便使数据在重新启动时始终散列到相同的位置
    // sort hosts so that data always hashes to the same place during restart
    std::sort(hosts.begin(), hosts.end());
    // remove rootdir suffix from host after sorting as no longer required
    for(auto& h : hosts) {
        auto idx = h.first.rfind("#");
        if(idx != string::npos)
            h.first.erase(idx, h.first.length());
    }
    return hosts;
}

} // namespace

namespace gaofs::utils {

optional<gaofs::metadata::Metadata> get_metadata(const string& path, bool follow_links) {
    string attr;
    auto err = gaofs::rpc::forward_stat(path, attr);
    if(err) {
        errno = err;
        return {};
    }

// has_symlink
#ifdef HAS_SYMLINKS
    if(follow_links) {
        gaofs::metadata::Metadata md{attr};
        while(md.is_link()) {
            err = gaofs::rpc::forward_stat(md.target_path(), attr);
            if(err) {
                errno = err;
                return {};
            }
            md = gaofs::metadata::Metadata{attr};
        }
    }
#endif
    return gaofs::metadata::Metadata{attr};
}

// 将metadata转变成 struct stat
int metadata_to_stat(const std::string& path, const gaofs::metadata::Metadata& md,
                 struct stat& attr) {
    // 填充默认值
    attr.st_dev = makedev(0, 0);
    attr.st_ino = std::hash<std::string>{}(path);
    attr.st_nlink = 1;
    attr.st_uid = CTX->fs_conf()->uid;
    attr.st_gid = CTX->fs_conf()->gid;
    attr.st_rdev = 0;
    attr.st_blksize = gaofs::config::rpc::chunksize;
    attr.st_blocks = 0;

    memset(&attr.st_atim, 0, sizeof(timespec));
    memset(&attr.st_mtim, 0, sizeof(timespec));
    memset(&attr.st_ctim, 0, sizeof(timespec));

    attr.st_mode = md.mode();
// HAS_SYMLINKS
#ifdef HAS_SYMLINKS
    if(md.is_link())
        attr.st_size = md.target_path().size() + CTX->mountdir().size();
    else
#endif
        attr.st_size = md.size();

    if(CTX->fs_conf()->atime_state) {
        attr.st_atim.tv_sec = md.atime();
    }
    if(CTX->fs_conf()->mtime_state) {
        attr.st_mtim.tv_sec = md.mtime();
    }
    if(CTX->fs_conf()->ctime_state) {
        attr.st_ctim.tv_sec = md.ctime();
    }
    if(CTX->fs_conf()->link_cnt_state) {
        attr.st_nlink = md.link_count();
    }
    if(CTX->fs_conf()->blocks_state) { // last one will not encounter a
        // delimiter anymore
        attr.st_blocks = md.blocks();
    }
    return 0;
}

vector<pair<string, string>> read_hosts_file() {
    string hostfile;

    // 获取hostfile路径
    hostfile = gaofs::env::get_var(gaofs::env::HOSTS_FILE, gaofs::config::hostfile_path);

    vector<pair<string, string>> hosts;
    try {
        hosts = load_hostfile(hostfile);
    } catch(const exception& e) {
        auto emsg = fmt::format("Failed to load hosts file: {}", e.what());
        throw runtime_error(emsg);
    }

    if(hosts.empty()) {
        throw runtime_error(fmt::format("Hostfile empty: '{}'", hostfile));
    }

    LOG(INFO, "Hosts pool size: {}", hosts.size());
    return hosts;
}

// 通过hostfile与其他主机的进程通过hermes连接
void connect_to_hosts(const vector<pair<string, string>>& hosts) {
    auto local_hostname = gaofs::rpc::get_my_hostname(true);
    bool local_host_found = false;

    vector<hermes::endpoint> addrs;
    addrs.resize(hosts.size());

    vector<uint64_t> host_ids(hosts.size());
    // populate vector with [0, ..., host_size - 1]
    ::iota(::begin(host_ids), ::end(host_ids), 0);
    /*
     * 洗牌主机以平衡地址查找到所有主机
     * 过多的并发查找发送到同一个主机可能会使服务器不堪重负，
     * 在地址查找时返回错误
     */

    ::random_device rd; // obtain a random number from hardware
    ::mt19937 g(rd());  // seed the random generator
    ::shuffle(host_ids.begin(), host_ids.end(), g); // Shuffle hosts vector 洗牌捏
    // 查找地址并将抽象服务器地址放入rpc_addresses中
    for(const auto& id : host_ids) {
        const auto& hostname = hosts.at(id).first;
        const auto& uri = hosts.at(id).second;

        addrs[id] = lookup_endpoint(uri);

        if(!local_host_found && hostname == local_hostname) {
            LOG(DEBUG, "Found local host: {}", hostname);
            CTX->local_host_id(id);
            local_host_found = true;
        }

        LOG(DEBUG, "Found peer: {}", addrs[id].to_string());
    }

    if(!local_host_found) {
        LOG(WARNING, "Failed to find local host. Using host '0' as local host");
        CTX->local_host_id(0);
    }

    CTX->hosts(addrs);
}

} // namespace gaofs::utils

