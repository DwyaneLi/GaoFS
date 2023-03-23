//
// Created by DELL on 2023/3/20.
//

#ifndef GAOFS_PRELOAD_CONTEXT_HPP
#define GAOFS_PRELOAD_CONTEXT_HPP

#include <hermes.hpp>
#include <map>
#include <mercury.h>
#include <memory>
#include <vector>
#include <string>
#include <bitset>

#include <config.hpp>

namespace gaofs {

// 前置声明
namespace filemap {
class OpenFileMap;
}

namespace rpc {
class Distributor;
}

namespace log {
struct logger;
}

namespace preload {

// 客户端文件系统信息
struct FsConfig {
    bool atime_state;
    bool mtime_state;
    bool ctime_state;
    bool link_cnt_state;
    bool blocks_state;

    uid_t uid;
    gid_t gid;

    std::string rootdir;
};

// 文件的状态
enum class RelativizeStatus {
    internal,
    external,
    fd_unknown,
    fd_not_a_dir
};

class PreloadContext {
    /* user fds / internal fds */
    static auto constexpr MIN_INTERNAL_FD = MAX_OPEN_FDS - MAX_INTERNAL_FDS;
    static auto constexpr MAX_USER_FDS = MIN_INTERNAL_FD;

private:
    PreloadContext();

    std::shared_ptr<gaofs::filemap::OpenFileMap> ofm_;
    std::shared_ptr<gaofs::rpc::Distributor> distributor_;
    std::shared_ptr<FsConfig> fs_conf_;

    std::string cwd_; // 当前目录
    std::vector<std::string> mountdir_components_; // 挂载目录的组成部分
    std::string mountdir_; // 挂载目录

    std::vector<hermes::endpoint> hosts_; // 服务器结点
    uint64_t local_host_id_;
    uint64_t fwd_host_id_;
    std::string rpc_protocol_;
    bool auto_sm_{false};

    bool interception_enabled_;

    // 位图，记录了internal_fd的情况
    std::bitset<MAX_INTERNAL_FDS> internal_fds_;
    mutable std::mutex internal_fds_mutex_; // 一个可变数据成员，永远不会是const，即使它是const 对象的成员。
    bool internal_fds_must_relocate_;
    std::bitset<MAX_USER_FDS> protected_fds_;
    std::string hostname;

public:
    static PreloadContext* getInstance() {
        static PreloadContext instance;
        return &instance;
    }

    PreloadContext(PreloadContext const&) = delete;

    void operator=(PreloadContext const&) = delete;

    void init_logging();

    RelativizeStatus relativize_fd_path(int dirfd, const char* raw_path,
                       std::string& relative_path, int flags = 0,
                       bool resolve_last_link = true) const;

    bool relativize_path(const char* raw_path, std::string& relative_path,
                         bool resolve_last_link = true) const;

    // getter and setter
    void mountdir(const std::string& path);

    const std::string& mountdir() const;

    const std::vector<std::string>& mountdir_components() const;

    void cwd(const std::string& path);

    const std::string& cwd() const;

    const std::vector<hermes::endpoint>& hosts() const;

    void hosts(const std::vector<hermes::endpoint>& addrs);

    void clear_hosts();

    uint64_t local_host_id() const;

    void local_host_id(uint64_t id);

    uint64_t fwd_host_id() const;

    void fwd_host_id(uint64_t id);

    const std::string& rpc_protocol() const;

    void rpc_protocol(const std::string& rpc_protocol);

    bool auto_sm() const;

    void auto_sm(bool auto_sm);

    const std::shared_ptr<gaofs::filemap::OpenFileMap>& file_map() const;

    void distributor(std::shared_ptr<gaofs::rpc::Distributor> distributor);

    std::shared_ptr<gaofs::rpc::Distributor> distributor() const;

    const std::shared_ptr<FsConfig>& fs_conf() const;

    void enable_interception();

    void disable_interception();

    bool interception_enabled() const;

    int register_internal_fd(int fd);

    void unregister_internal_fd(int fd);

    bool is_internal_fd(int fd) const;

    void protect_user_fds();

    void unprotect_user_fds();

    std::string get_hostname();

};

} // namespace preload

} // namespace gaofs

#endif //GAOFS_PRELOAD_CONTEXT_HPP
