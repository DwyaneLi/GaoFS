//
// Created by DELL on 2023/3/20.
//

#include <client/preload_context.hpp>
#include <client/env.hpp>
#include <client/logging.hpp>
#include <client/open_file_map.hpp>
#include <client/open_dir.hpp>
#include <client/path.hpp>

#include <common/env_util.hpp>
#include <common/path_util.hpp>
#include <config.hpp>

#include <hermes.hpp>
#include <cassert>

extern "C" {
#include <libsyscall_intercept_hook_point.h>
#include <syscall.h>
}

namespace gaofs {

namespace preload {

PreloadContext::PreloadContext() : ofm_(std::make_shared<gaofs::filemap::OpenFileMap>()),
                            fs_conf_(std::make_shared<FsConfig>()) {
    internal_fds_.set();
    internal_fds_must_relocate_ = true;

    char host[255];
    gethostname(host, 255);
    hostname = host;
}

void PreloadContext::init_logging() {

    const std::string log_opts = gaofs::env::get_var(
            gaofs::env::LOG, gaofs::config::log::client_log_level);

    const std::string log_output = gaofs::env::get_var(
            gaofs::env::LOG_OUTPUT, gaofs::config::log::client_log_path);

    // TODO: GAOFS_DEBUG_BUILD
#ifdef GAOFS_DEBUG_BUILD
    // atoi returns 0 if no int conversion can be performed, which works
    // for us since if the user provides a non-numeric value we can just treat
    // it as zero
    const int log_verbosity = std::atoi(
            gaofs::env::get_var(gaofs::env::LOG_DEBUG_VERBOSITY, "0").c_str());

    const std::string log_filter =
            gaofs::env::get_var(gaofs::env::LOG_SYSCALL_FILTER, "");
#endif

    const std::string trunc_val =
            gaofs::env::get_var(gaofs::env::LOG_OUTPUT_TRUNC);

    const bool log_trunc = (!trunc_val.empty() && trunc_val[0] != '0');

    gaofs::log::create_global_logger(log_opts, log_output, log_trunc
#ifdef GAOFS_DEBUG_BUILD
            ,
                                    log_filter, log_verbosity
#endif
    );
}

void PreloadContext::mountdir(const std::string &path) {
    assert(gaofs::path::is_absolute(path));
    assert(!gaofs::path::has_trailing_slash(path));
    mountdir_ = path;
    mountdir_components_ = gaofs::path::split_path(path);
}

// getter and setter
const std::string& PreloadContext::mountdir() const {
    return mountdir_;
}

const std::vector<std::string>& PreloadContext::mountdir_components() const {
    return mountdir_components_;
}

void PreloadContext::cwd(const std::string& path) {
    cwd_ = path;
}

const std::string& PreloadContext::cwd() const {
    return cwd_;
}

const std::vector<hermes::endpoint>& PreloadContext::hosts() const {
    return hosts_;
}

void PreloadContext::hosts(const std::vector<hermes::endpoint>& endpoints) {
    hosts_ = endpoints;
}

void PreloadContext::clear_hosts() {
    hosts_.clear();
}

uint64_t PreloadContext::local_host_id() const {
    return local_host_id_;
}

void PreloadContext::local_host_id(uint64_t id) {
    local_host_id_ = id;
}

uint64_t PreloadContext::fwd_host_id() const {
    return fwd_host_id_;
}

void PreloadContext::fwd_host_id(uint64_t id) {
    fwd_host_id_ = id;
}

const std::string& PreloadContext::rpc_protocol() const {
    return rpc_protocol_;
}

void PreloadContext::rpc_protocol(const std::string& rpc_protocol) {
    rpc_protocol_ = rpc_protocol;
}

bool PreloadContext::auto_sm() const {
    return auto_sm_;
}

void PreloadContext::auto_sm(bool auto_sm) {
    PreloadContext::auto_sm_ = auto_sm;
}

const std::shared_ptr<gaofs::filemap::OpenFileMap>& PreloadContext::file_map() const {
    return ofm_;
}

void PreloadContext::distributor(std::shared_ptr<gaofs::rpc::Distributor> d) {
    distributor_ = d;
}

std::shared_ptr<gaofs::rpc::Distributor> PreloadContext::distributor() const {
    return distributor_;
}

const std::shared_ptr<FsConfig>& PreloadContext::fs_conf() const {
    return fs_conf_;
}

void PreloadContext::enable_interception() {
    interception_enabled_ = true;
}

void PreloadContext::disable_interception() {
    interception_enabled_ = false;
}

bool PreloadContext::interception_enabled() const {
    return interception_enabled_;
}

std::string PreloadContext::get_hostname() {
    return hostname;
}

// 判断一个文件的状态，是否在挂载的目录内
// 并且填充relative_path 也就是相对于挂载路径的路径
// 第一个参数众所周知是一个文件描述符, 该参数若使用标志AT_FDCWD, 则raw_path参数使用的是相对路径,
// 如果raw_path是一个绝对路径, 则忽略fd参数,
RelativizeStatus PreloadContext::relativize_fd_path(int dirfd, const char *raw_path, std::string &relative_path,
                                                    int flags, bool resolve_last_link) const {

    // 库构造函数调用后才可以执行
    assert(interception_enabled_);
    assert(!mountdir_.empty());
    assert(raw_path != nullptr);

    std::string path;

    if(raw_path != nullptr && raw_path[0] != gaofs::path::separator) {
        // path is relative
        if(dirfd == AT_FDCWD) {
            // 在当前工作目录下
            path = gaofs::path::prepend_path(cwd_, raw_path);
        } else {
            if(!ofm_->exist(dirfd)) {
                // 不存在该目录
                return RelativizeStatus::fd_unknown;
            } else {
                // check if we have the AT_EMPTY_PATH flag
                // for fstatat.
                if(flags & AT_EMPTY_PATH) {
                    relative_path = ofm_->get(dirfd)->path();
                    return RelativizeStatus::internal;
                }
            }
            // path is relative to fd
            auto dir = ofm_->get_dir(dirfd);
            if(dir == nullptr) {
                return RelativizeStatus::fd_not_a_dir;
            }
            path = mountdir_;
            path.append(dir->path());
            path.push_back(gaofs::path::separator);
            path.append(raw_path);
        }
    } else {
        // raw_path 是绝对路径
        path = raw_path;
    }

    if(gaofs::path::resolve(path, relative_path, resolve_last_link)) {
        return RelativizeStatus::internal;
    }
    return RelativizeStatus::external;
}

// 和上面那个差不多，也就是默认dirfd是cwd
bool PreloadContext::relativize_path(const char *raw_path, std::string &relative_path, bool resolve_last_link) const {

        assert(interception_enabled_);
        assert(!mountdir_.empty());
        assert(raw_path != nullptr);

        std::string path;

        if(raw_path != nullptr && raw_path[0] != gaofs::path::separator) {
            /* Path is not absolute, we need to prepend CWD;
             * First reserve enough space to minimize memory copy
             */
            path = gaofs::path::prepend_path(cwd_, raw_path);
        } else {
            path = raw_path;
        }

        return gaofs::path::resolve(path, relative_path, resolve_last_link);
}


//将fd注册，如果internal_fds_must_relocate_为true的话，则要将fd重新变成另一个值
int PreloadContext::register_internal_fd(int fd) {
    assert(fd >= 0);

    if(!internal_fds_must_relocate_) {
        LOG(DEBUG, "registering fd {} as internal (no relocation needed)", fd);
        assert(fd >= MIN_INTERNAL_FD);
        internal_fds_.reset(fd - MIN_INTERNAL_FD);
        return fd;
    }

    LOG(DEBUG, "registering fd {} as internal (needs relocation)", fd);

    std::lock_guard<std::mutex> lock(internal_fds_mutex_);
    const int pos = internal_fds_._Find_first();

    if(static_cast<std::size_t>(pos) == internal_fds_.size()) {
        throw std::runtime_error(
                "Internal GaoFS file descriptors exhausted, increase MAX_INTERNAL_FDS in "
                "CMake, rebuild GaoFS and try again.");
    }
    internal_fds_.reset(pos);

    // TODO GAOS_ENABLE_LOGGING GAOFS_DEBUG_BUILD
#if defined(GAOS_ENABLE_LOGGING) && defined(GAOFS_DEBUG_BUILD)
    long args[gaofs::syscall::MAX_ARGS]{fd, pos + MIN_INTERNAL_FD, O_CLOEXEC};
#endif

    LOG(SYSCALL,
        gaofs::syscall::from_internal_code | gaofs::syscall::to_kernel |
        gaofs::syscall::not_executed,
        SYS_dup3, args);

    const int ifd = ::syscall_no_intercept(SYS_dup3, fd, pos + MIN_INTERNAL_FD, O_CLOEXEC);
        LOG(SYSCALL,
            gaofs::syscall::from_internal_code | gaofs::syscall::to_kernel |
            gaofs::syscall::executed,
            SYS_dup3, args, ifd);

    assert(::syscall_error_code(ifd) == 0);

// TODO GAOS_ENABLE_LOGGING GAOFS_DEBUG_BUILD
#if defined(GAOFS_ENABLE_LOGGING) && defined(GAOFS_DEBUG_BUILD)
    long args2[gaofs::syscall::MAX_ARGS]{fd};
#endif

    LOG(SYSCALL,
        gaofs::syscall::from_internal_code | gaofs::syscall::to_kernel |
        gaofs::syscall::not_executed,
        SYS_close, args2);

// TODO GAOS_ENABLE_LOGGING GAOFS_DEBUG_BUILD
#if defined(GAOFS_ENABLE_LOGGING) && defined(GAOFS_DEBUG_BUILD)
    int rv = ::syscall_no_intercept(SYS_close, fd);
#else
    ::syscall_no_intercept(SYS_close, fd);
#endif

    LOG(SYSCALL,
        gaofs::syscall::from_internal_code | gaofs::syscall::to_kernel |
        gaofs::syscall::executed,
        SYS_close, args2, rv);

    LOG(DEBUG, "    (fd {} relocated to ifd {})", fd, ifd);

    return ifd;
}

void PreloadContext::unregister_internal_fd(int fd) {
    LOG(DEBUG, "unregistering internal fd {}", fd);

    assert(fd >= MIN_INTERNAL_FD);
     const auto pos = fd - MIN_INTERNAL_FD;

     std::lock_guard<std::mutex> lock(internal_fds_mutex_);
     internal_fds_.set(pos);
}

// 判断是否为内部文件的文件描述符
bool PreloadContext::is_internal_fd(int fd) const {

    if(fd < MIN_INTERNAL_FD) {
        return false;
    }

    const auto pos = fd - MIN_INTERNAL_FD;

    std::lock_guard<std::mutex> lock(internal_fds_mutex_);
    return !internal_fds_.test(pos);
}

// 强制占用[0，max_user_fds)
void PreloadContext::protect_user_fds() {
    LOG(DEBUG, "Protecting application fds [{}, {}]", 0, MAX_USER_FDS - 1);

    const int nullfd = ::syscall_no_intercept(SYS_open, "/dev/null", O_RDONLY);

    const auto fd_is_open = [](int fd) -> bool {
        const int ret = ::syscall_no_intercept(SYS_fcntl, fd, F_GETFD);
        const int error = ::syscall_error_code(ret);
        return error == 0 || error != EBADF;
    };

    for(int fd = 0; fd < MAX_USER_FDS; ++fd) {
        if(fd_is_open(fd)) {
            LOG(DEBUG, "  fd {} was already in use, skipping", fd);
            continue;
        }

        const int ret = ::syscall_no_intercept(SYS_dup3, nullfd, fd, O_CLOEXEC);
        assert(::syscall_error_code(ret) == 0);
        protected_fds_.set(fd);
    }

    internal_fds_must_relocate_ = false;
}

void PreloadContext::unprotect_user_fds() {

    for(std::size_t fd = 0; fd < protected_fds_.size(); ++fd) {
        if(!protected_fds_[fd]) {
            continue;
        }

        const int ret =
                ::syscall_error_code(::syscall_no_intercept(SYS_close, fd));

        if(ret != 0) {
            LOG(ERROR, "Failed to unprotect fd")
        }
    }

    internal_fds_must_relocate_ = true;
}

} // namespace preload

} // namespace gaofs

