//
// Created by DELL on 2023/3/20.
//
#include <client/preload.hpp>
#include <client/path.hpp>
#include <client/logging.hpp>
#include <client/rpc/forward_management.hpp>
#include <client/preload_util.hpp>
#include <client/intercept.hpp>

#include <common/rpc/distributor.hpp>
#include <common/common_defs.hpp>

#include <fstream>
#include <hermes.hpp>

using namespace std;

// 外部变量，用于rpc传输
std::unique_ptr<hermes::async_engine> ld_network_service;

namespace {

// todo:  GAOFS_ENABLE_FORWARDING
#ifdef GAOFS_ENABLE_FORWARDING
pthread_t mapper;
bool forwarding_running;

pthread_mutex_t remap_mutex;
pthread_cond_t remap_signal;
#endif

inline void exit_error_msg(int errcode, const string& msg) {

    LOG_ERROR("{}", msg);
    gaofs::log::logger::log_message(stderr, "{}\n", msg);

    // if we don't disable interception before calling ::exit()
    // syscall hooks may find an inconsistent in shared state
    // (e.g. the logger) and thus, crash
    // lxl
    // 如果我们在调用::exit()之前不禁用拦截，系统调用钩子可能会在
    // 共享状态(例如记录器)中发现不一致，从而崩溃
    gaofs::preload::stop_interception();
    CTX->disable_interception();
    ::exit(errcode);
}

// 初始化hermes客户端, 包括设置ld_network_service和传输协议
bool init_hermes_client() {

    try {

        hermes::engine_options opts{};
        //设置传输协议
        if(CTX->auto_sm())
            opts |= hermes::use_auto_sm;
        if(gaofs::rpc::protocol::ofi_psm2 == CTX->rpc_protocol()) {
            opts |= hermes::force_no_block_progress;
        }

        opts |= hermes::process_may_fork;

        ld_network_service = std::make_unique<hermes::async_engine>(
                hermes::get_transport_type(CTX->rpc_protocol()), opts);
        ld_network_service->run();
    } catch(const std::exception& ex) {
        fmt::print(stderr, "Failed to initialize Hermes RPC client {}\n",
                   ex.what());
        return false;
    }
    return true;
}

// GAOFS_ENABLE_FORWARDING
#ifdef GAOFS_ENABLE_FORWARDING
void* forwarding_mapper(void* p) {
    struct timespec timeout;
    clock_gettime(CLOCK_REALTIME, &timeout);
    timeout.tv_sec += 10; // 10 seconds

    int previous = -1;

    while(forwarding_running) {
        try {
            gaofs::utils::load_forwarding_map();

            if(previous != CTX->fwd_host_id()) {
                LOG(INFO, "{}() Forward to {}", __func__, CTX->fwd_host_id());

                previous = CTX->fwd_host_id();
            }
        } catch(std::exception& e) {
            exit_error_msg(EXIT_FAILURE,
                           fmt::format("Unable set the forwarding host '{}'",
                                       e.what()));
        }

        pthread_mutex_lock(&remap_mutex);
        pthread_cond_timedwait(&remap_signal, &remap_mutex, &timeout);
        pthread_mutex_unlock(&remap_mutex);
    }

    return nullptr;
}

void init_forwarding_mapper() {
    forwarding_running = true;

    pthread_create(&mapper, NULL, forwarding_mapper, NULL);
}

void destroy_forwarding_mapper() {
    forwarding_running = false;

    pthread_cond_signal(&remap_signal);

    pthread_join(mapper, NULL);
}
#endif

void log_prog_name() {
    std::string line;
    std::ifstream cmdline("/proc/self/cmdline");
    if(!cmdline.is_open()) {
        LOG(ERROR, "Unable to open cmdline file");
        throw std::runtime_error("Unable to open cmdline file");
    }
    if(!getline(cmdline, line)) {
        throw std::runtime_error("Unable to read cmdline file");
    }
    std::replace(line.begin(), line.end(), '\0', ' ');
    line.erase(line.length() - 1, line.length());
    LOG(INFO, "Process cmdline: '{}'", line);
    cmdline.close();
}

} // namespace

namespace gaofs::preload {

// 此函数只在预加载构造函数中调用，并初始化文件系统客户端
void init_environment() {

    // 初始化hosts，并连接各个节点
    vector<pair<string, string>> hosts{};
    try {
        LOG(INFO, "Loading peer addresses...");
        hosts = gaofs::utils::read_hosts_file();
    } catch(const std::exception& e) {
        exit_error_msg(EXIT_FAILURE,
                       "Failed to load hosts addresses: "s + e.what());
    }

    // initialize Hermes interface to Mercury
    LOG(INFO, "Initializing RPC subsystem...");

    if(!init_hermes_client()) {
        exit_error_msg(EXIT_FAILURE, "Unable to initialize RPC subsystem");
    }

    try {
        gaofs::utils::connect_to_hosts(hosts);
    } catch(const std::exception& e) {
        exit_error_msg(EXIT_FAILURE,
                       "Failed to connect to hosts: "s + e.what());
    }

    // 根据不同的宏定义设置distributor
#ifdef GAOFS_ENABLE_FORWARDING
    try {
        gaofs::utils::load_forwarding_map();

        LOG(INFO, "{}() Forward to {}", __func__, CTX->fwd_host_id());
    } catch(std::exception& e) {
        exit_error_msg(
                EXIT_FAILURE,
                fmt::format("Unable set the forwarding host '{}'", e.what()));
    }

    auto forwarder_dist = std::make_shared<gaofs::rpc::ForwarderDistributor>(
            CTX->fwd_host_id(), CTX->hosts().size());
    CTX->distributor(forwarder_dist);
#else
#ifdef GAOFS_USE_GUIDED_DISTRIBUTION
    auto distributor = std::make_shared<gaofs::rpc::GuidedDistributor>(
            CTX->local_host_id(), CTX->hosts().size());
#else
    auto distributor = std::make_shared<gaofs::rpc::SimpleHashDistributor>(
                CTX->local_host_id(), CTX->hosts().size());
#endif
    CTX->distributor(distributor);
#endif

    LOG(INFO, "Retrieving file system configuration...");
    // 检查是否可以获取服务端配置
    if(!gaofs::rpc::forward_get_fs_config()) {
        exit_error_msg(
                EXIT_FAILURE,
                "Unable to fetch file system configurations from daemon process through RPC.");
    }

    LOG(INFO, "Environment initialization successful.");
}

} // namespace gaofs::preload

// 在与LD_PRELOAD环境变量一起使用preload库时，最初调用一次,初始化preload环境，配置拦截函数
void init_preload() {
    // 初始化后将恢复原始的errno值，以避免泄漏内部错误码
    auto oerrno = errno;

    CTX->enable_interception();
    // 使用hook_internal配置环境
    gaofs::preload::start_self_interception();

    // 初始化日志
    CTX->init_logging();
    LOG(DEBUG, "Logging subsystem initialized");
    // 如ib_uverbs这样的内核模块可以在内核空间中创建fds，并使用类似ioctl()的接口将
    // 它们传递给用户空间进程。如果这发生在我们的内部初始化过程中，我们没有办法控制这
    // 个创建过程，fd将被创建在[0,MAX_USER_FDS)范围内，而不是在我们的私有[MAX_USER_FDS, MAX_OPEN_FDS)范围内。
    // 为了防止内部初始化代码出现这种情况，我们强制占用用户fd范围，以迫使这些模块在我们的私有范围内创建fds。
    CTX->protect_user_fds();

    log_prog_name();
    gaofs::path::init_cwd();

    LOG(DEBUG, "Current working directory: '{}'", CTX->cwd());
    gaofs::preload::init_environment();
    CTX->enable_interception();

    // lxl 初始化完之后再释放
    CTX->unprotect_user_fds();

#ifdef GAOFS_ENABLE_FORWARDING
    init_forwarding_mapper();
#endif

    gaofs::preload::start_interception();
    errno = oerrno;
}

// 释放资源
void destroy_preload() {
#ifdef GAOFS_ENABLE_FORWARDING
    destroy_forwarding_mapper();
#endif

    CTX->clear_hosts();
    LOG(DEBUG, "Peer information deleted");

    ld_network_service.reset();
    LOG(DEBUG, "RPC subsystem shut down");

    gaofs::preload::stop_interception();
    CTX->disable_interception();
    LOG(DEBUG, "Syscall interception stopped");

    LOG(INFO, "All subsystems shut down. Client shutdown complete.");
}