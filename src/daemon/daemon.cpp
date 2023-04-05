//
// Created by DELL on 2023/1/16.
//

#include <daemon/daemon.hpp>

#include <filesystem>
#include <iostream>
#include <fstream>
#include <csignal>
#include <condition_variable>
#include <tuple>


extern "C" {
#include <unistd.h>
#include <cstdlib>
}

#include <version.hpp>
#include <common/log_util.hpp>
#include <common/env_util.hpp>
#include <common/rpc/rpc_types.hpp>
#include <common/rpc/rpc_util.hpp>

#include <daemon/env.hpp>
#include <daemon/handler/rpc_defs.hpp>
#include <daemon/ops/metadentry.hpp>
#include <daemon/backend/metadata/db.hpp>
#include <daemon/backend/data/chunk_storage.hpp>
#include <daemon/util.hpp>
#include <CLI11/CLI11.hpp>

using namespace std;
namespace fs = std::filesystem;

// 关闭型号
static condition_variable shutdown_please;
// shutdown_please的互斥锁
static mutex mtx;
static bool keep_rootdir = true;

// 客户端选项
struct cli_options {
    string mountdir;
    string rootdir;
    string rootdir_suffix;
    string metadir;
    string listen;
    string hosts_file;
    string rpc_protocol;
    string dbbackend;
    string parallax_size;
    string stats_file;
    string prometheus_gateway;
};

/*
void init_environment_test() {
    std::string metadata_path = "/home/dwyaneli/gaofs_metadata";
    fs::create_directories(metadata_path);

    // 初始化数据库
    GAOFS_DATA->dbbackend(gaofs::metadata::rocksdb_backend);
    try {
        std::string_view mydbbackend = GAOFS_DATA->dbbackend();
        std::shared_ptr<gaofs::metadata::MetadataDB> mydb = std::make_shared<gaofs::metadata::MetadataDB>(
                metadata_path, mydbbackend);
        GAOFS_DATA->mdb(mydb);
    } catch(const std::exception& e) {
        std::cout << e.what() << std::endl;
        throw;
    }
    std::cout << "lalal new" << std::endl;
    // 根据config初始化选项
    GAOFS_DATA->atime_state(gaofs::config::metadata::use_atime);
    GAOFS_DATA->ctime_state(gaofs::config::metadata::use_ctime);
    GAOFS_DATA->mtime_state(gaofs::config::metadata::use_mtime);
    GAOFS_DATA->link_cnt_state(gaofs::config::metadata::use_link_cnt);
    GAOFS_DATA->blocks_state(gaofs::config::metadata::use_blocks);

    // 为根目录创建原条目, 同时也在测试create
    gaofs::metadata::Metadata root_md{S_IFDIR | S_IRWXU | S_IRWXG | S_IRWXO};
    gaofs::metadata::create("/", root_md);

    gaofs::metadata::Metadata _m_md;
    _m_md.size(500);
    _m_md.mode(S_IFDIR | S_IRWXU | S_IRWXG | S_IRWXO);
    gaofs::metadata::create("/_m", _m_md);

    gaofs::metadata::Metadata _m_m_md;
    _m_m_md.size(600);
    _m_m_md.mode(S_IFDIR | S_IRWXU | S_IRWXG | S_IRWXO);
    gaofs::metadata::create("/_m_m", _m_m_md);

    gaofs::metadata::Metadata _m_1_md;
    _m_1_md.size(700);
    _m_1_md.mode(S_IRWXU | S_IRWXG | S_IRWXO);
    gaofs::metadata::create("/_m/1", _m_1_md);

    gaofs::metadata::Metadata _m_2_md;
    _m_2_md.size(800);
    _m_2_md.mode(S_IRWXU | S_IRWXG | S_IRWXO);
    gaofs::metadata::create("/_m/2", _m_2_md);

    gaofs::metadata::Metadata _m_m_1_md;
    _m_m_1_md.size(900);
    _m_m_1_md.mode(S_IRWXU | S_IRWXG | S_IRWXO);
    gaofs::metadata::create("/_m_m/1", _m_m_1_md);

    gaofs::metadata::Metadata _m_m_2_md;
    _m_m_2_md.size(1000);
    _m_m_2_md.mode(S_IRWXU | S_IRWXG | S_IRWXO);
    gaofs::metadata::create("/_m_m/2", _m_m_2_md);

    // 测试get_size and get
    std::cout << "----------------------------------test get_size and get---------------------------------------" << std::endl;
    auto _m_size = gaofs::metadata::get_size("/_m");
    auto _m_1_size = gaofs::metadata::get_size("/_m/1");
    auto _m_2_size = gaofs::metadata::get_size("/_m/2");
    auto _m_m_size = gaofs::metadata::get_size("/_m_m");
    auto _m_m_1_size = gaofs::metadata::get_size("/_m_m/1");
    auto _m_m_2_size = gaofs::metadata::get_size("/_m_m/2");
    std::cout << "/_m: " << _m_size << "\n"
              << "/_m/1: " << _m_1_size << "\n"
              << "/_m/2: " << _m_2_size << "\n"
              << "/_m_m: " << _m_m_size << "\n"
              << "/_m_m/1: " << _m_m_1_size << "\n"
              << "/_m_m/2: " << _m_m_2_size << "\n\n" << std::endl;


    // 测试update
    std::cout << "----------------------------------update---------------------------------------" << std::endl;
    _m_md.size(1500);
    _m_m_md.size(1600);
    _m_1_md.size(1700);
    _m_2_md.size(1800);
    _m_m_1_md.size(1900);
    _m_m_2_md.size(2000);
    gaofs::metadata::update("/_m", _m_md);
    gaofs::metadata::update("/_m_m", _m_m_md);
    gaofs::metadata::update("/_m/1", _m_1_md);
    gaofs::metadata::update("/_m/2", _m_2_md);
    gaofs::metadata::update("/_m_m/1", _m_m_1_md);
    gaofs::metadata::update("/_m_m/2", _m_m_2_md);
    _m_size = gaofs::metadata::get_size("/_m");
    _m_1_size = gaofs::metadata::get_size("/_m/1");
    _m_2_size = gaofs::metadata::get_size("/_m/2");
    _m_m_size = gaofs::metadata::get_size("/_m_m");
    _m_m_1_size = gaofs::metadata::get_size("/_m_m/1");
    _m_m_2_size = gaofs::metadata::get_size("/_m_m/2");
    std::cout << "/_m: " << _m_size << "\n"
              << "/_m/1: " << _m_1_size << "\n"
              << "/_m/2: " << _m_2_size << "\n"
              << "/_m_m: " << _m_m_size << "\n"
              << "/_m_m/1: " << _m_m_1_size << "\n"
              << "/_m_m/2: " << _m_m_2_size << "\n\n" << std::endl;

    // 测试update size
    std::cout << "----------------------------------update size---------------------------------------" << std::endl;
    gaofs::metadata::update_size("/_m", 1000, 0, true);
    gaofs::metadata::update_size("/_m_m", 1000, 0, true);
    gaofs::metadata::update_size("/_m/1", 1000, 0, true);
    gaofs::metadata::update_size("/_m/2", 1000, 0, true);
    gaofs::metadata::update_size("/_m_m/1", 1000, 0, true);
    gaofs::metadata::update_size("/_m_m/2", 1000, 0, true);
    _m_size = gaofs::metadata::get_size("/_m");
    _m_1_size = gaofs::metadata::get_size("/_m/1");
    _m_2_size = gaofs::metadata::get_size("/_m/2");
    _m_m_size = gaofs::metadata::get_size("/_m_m");
    _m_m_1_size = gaofs::metadata::get_size("/_m_m/1");
    _m_m_2_size = gaofs::metadata::get_size("/_m_m/2");
    std::cout << "/_m: " << _m_size << "\n"
              << "/_m/1: " << _m_1_size << "\n"
              << "/_m/2: " << _m_2_size << "\n"
              << "/_m_m: " << _m_m_size << "\n"
              << "/_m_m/1: " << _m_m_1_size << "\n"
              << "/_m_m/2: " << _m_m_2_size << "\n\n" << std::endl;

    // 测试remove
    std::cout << "----------------------------------remove---------------------------------------" << std::endl;
    gaofs::metadata::remove("/_m_m");
    // std::cout << "/_m_m: " << gaofs::metadata::get_size("/_m_m") << "\n" << std::endl;
    _m_m_md.size(2600);
    gaofs::metadata::create("/_m_m", _m_m_md);

    // 测试create_first_chunk
    gaofs::metadata::First_chunk _m_1_f;
    _m_1_f.size(700);
    _m_1_f.content("this is /_m/1");
    gaofs::metadata::create_first_chunk("/_m/1", _m_1_f);
    gaofs::metadata::First_chunk _m_2_f;
    _m_2_f.size(800);
    _m_2_f.content("this is /_m/2");
    gaofs::metadata::create_first_chunk("/_m/2", _m_2_f);

    gaofs::metadata::Metadata _f_md;
    _f_md.size(2100);
    _f_md.mode(S_IRWXU | S_IRWXG | S_IRWXO);
    gaofs::metadata::create("/_f", _f_md);
    gaofs::metadata::First_chunk _f_f;
    _f_f.size(900);
    _f_f.content("this is /_f");
    gaofs::metadata::create_first_chunk("/_f", _f_f);

    // 测试get_first_chunk
    std::cout << "----------------------------------test get_size and get(first chunk)---------------------------------------" << std::endl;
    auto _m_1_f_size = gaofs::metadata::get_size_first_chunk("/_m/1");
    auto _m_2_f_size = gaofs::metadata::get_size_first_chunk("/_m/2");
    auto _f_f_size = gaofs::metadata::get_size_first_chunk("/_f");
    std::cout << "/_m/1 firstchunk : " << _m_1_f_size << "\n"
              << "/_m/2 firstchunk : " << _m_2_f_size << "\n"
              << "/_f firstchunk : " << _f_f_size << "\n\n" << std::endl;


    // 测试get_first_chunk_content
    std::cout << "----------------------------------test get_content and get(first_chunk)---------------------------------------" << std::endl;
    auto _m_1_f_c = gaofs::metadata::get_content_first_chunk("/_m/1");
    auto _m_2_f_c = gaofs::metadata::get_content_first_chunk("/_m/2");
    auto _f_f_c = gaofs::metadata::get_content_first_chunk("/_f");
    std::cout << "/_m/1 firstchunk : " << _m_1_f_c << "\n"
              << "/_m/2 firstchunk : " << _m_2_f_c << "\n"
              << "/_f firstchunk : " << _f_f_c << "\n\n" << std::endl;

    // 测试update_first_chunk
    std::cout << "----------------------------------test update(first_chunk)---------------------------------------" << std::endl;
    _m_1_f.size(1700);
    _m_2_f.size(1800);
    _f_f.size(1900);
    gaofs::metadata::update_first_chunk("/_m/1", _m_1_f);
    gaofs::metadata::update_first_chunk("/_m/2", _m_2_f);
    gaofs::metadata::update_first_chunk("/_f", _f_f);
    _m_1_f_size = gaofs::metadata::get_size_first_chunk("/_m/1");
    _m_2_f_size = gaofs::metadata::get_size_first_chunk("/_m/2");
    _f_f_size = gaofs::metadata::get_size_first_chunk("/_f");
    std::cout << "/_m/1 firstchunk : " << _m_1_f_size << "\n"
              << "/_m/2 firstchunk : " << _m_2_f_size << "\n"
              << "/_f firstchunk : " << _f_f_size << "\n\n" << std::endl;

    // 测试remove_first_chunk
    std::cout << "----------------------------------test remove(first_chunk)---------------------------------------" << std::endl;
    gaofs::metadata::remove_first_chunk("/_m/1");
    // std::cout << "/_m/1 firstchunk : " << gaofs::metadata::get_size_first_chunk("/_m/1");
    _m_1_f.size(1700);
    _m_1_f.content("this is /_m/1");
    gaofs::metadata::create_first_chunk("/_m/1", _m_1_f);

    // 测试get_dirents
    std::cout << "----------------------------------test get dirents---------------------------------------" << std::endl;
    auto root_dirents = gaofs::metadata::get_dirents("/");
    auto _m_dirents = gaofs::metadata::get_dirents("/_m");
    auto _m_m_dirents = gaofs::metadata::get_dirents("/_m_m");
    std::cout << "/:" << std::endl;
    for (int i = 0; i < root_dirents.size(); i++) {
        std::cout << root_dirents[i].first << " " << root_dirents[i].second << std::endl;
    }
    std::cout << "-------------" << std::endl;
    std::cout << "/_m:" << std::endl;
    for (int i = 0; i < _m_dirents.size(); i++) {
        std::cout << _m_dirents[i].first << " " << _m_dirents[i].second << std::endl;
    }
    std::cout << "-------------" << std::endl;
    std::cout << "/_m_m:" << std::endl;
    for (int i = 0; i < _m_m_dirents.size(); i++) {
        std::cout << _m_m_dirents[i].first << " " << _m_m_dirents[i].second << std::endl;
    }
    std::cout << "-------------\n" << std::endl;

    // 测试get_dirents_extended
    std::cout << "----------------------------------test get dirents_extened---------------------------------------" << std::endl;
    auto root_dirents_e = gaofs::metadata::get_dirents_extended("/");
    auto _m_dirents_e = gaofs::metadata::get_dirents_extended("/_m");
    auto _m_m_dirents_e = gaofs::metadata::get_dirents_extended("/_m_m");
    std::cout << "/:" << std::endl;
    for (int i = 0; i < root_dirents_e.size(); i++) {
        std::cout << std::get<0>(root_dirents_e[i]) << " " << std::get<2>(root_dirents_e[i]) << std::endl;
    }
    std::cout << "-------------" << std::endl;
    std::cout << "/_m:" << std::endl;
    for (int i = 0; i < _m_dirents_e.size(); i++) {
        std::cout << std::get<0>(_m_dirents_e[i]) << " " << std::get<2>(_m_dirents_e[i]) << std::endl;
    }
    std::cout << "-------------" << std::endl;
    std::cout << "/_m_m:" << std::endl;
    for (int i = 0; i < _m_m_dirents_e.size(); i++) {
        std::cout << std::get<0>(_m_m_dirents_e[i]) << " " << std::get<2>(_m_m_dirents_e[i]) << std::endl;
    }
    std::cout << "-------------" << std::endl;
    std::cout << "\n" << std::endl;

    // 测试iterate_all
    std::cout << "----------------------------------iterate_all---------------------------------------" << std::endl;
    gaofs::metadata::iterate_all();

}
*/

// 为非阻塞I/O初始化Argobots执行流
// 相应的执行流数量在gaofs::config::rpc::daemon_io_xstreams中定义。FIFO线程池容纳这些执行流。
// Argobots微线程是在I/O操作期间从这些池创建的
// 初始化线程池
void init_io_tasklet_pool() {
    static_assert(gaofs::config::rpc::daemon_io_xstreams >= 0,
                  "Daemon IO Execution streams must be higher than 0!");
    // 获取执行流数目
    unsigned int xstreams_num = gaofs::config::rpc::daemon_io_xstreams;

    // 检索刚刚创建的调度程序的池 其实就是创建任务调度的池
    ABT_pool pool;
    auto ret = ABT_pool_create_basic(ABT_POOL_FIFO_WAIT, ABT_POOL_ACCESS_MPMC,
                                     ABT_TRUE, &pool);
    if(ret != ABT_SUCCESS) {
        throw runtime_error("Failed to create I/O tasks pool");
    }
    // 创建所有后续的xstream和相关的调度器，所有都进入相同的池
    vector<ABT_xstream> xstreams(xstreams_num);
    for(unsigned int i = 0; i < xstreams_num; ++i) {
        ret = ABT_xstream_create_basic(ABT_SCHED_BASIC_WAIT, 1, &pool,
                                       ABT_SCHED_CONFIG_NULL, &xstreams[i]);
        if(ret != ABT_SUCCESS) {
            throw runtime_error(
                    "Failed to create task execution streams for I/O operations");
        }
    }

    // 设置rpc_data
    RPC_DATA->io_streams(xstreams);
    RPC_DATA->io_pool(pool);
}

// 将RPC处理程序注册到给定的Margo实例
// 注册是通过将Margo实例id(mid)与RPC名称及其处理函数(包括定义的输入/输出结构)相关联来完成的
// mid rpc名称 输入 输出 处理函数名
// symlinks
void register_server_rpcs(margo_instance_id mid) {
    MARGO_REGISTER(mid, gaofs::rpc::tag::fs_config, void, rpc_config_out_t, rpc_srv_get_fs_config);
    MARGO_REGISTER(mid, gaofs::rpc::tag::create, rpc_mk_node_in_t, rpc_err_out_t, rpc_srv_create);
    MARGO_REGISTER(mid, gaofs::rpc::tag::stat, rpc_path_only_in_t,
                   rpc_stat_out_t, rpc_srv_stat);
    MARGO_REGISTER(mid, gaofs::rpc::tag::decr_size, rpc_trunc_in_t,
                   rpc_err_out_t, rpc_srv_decr_size);
    MARGO_REGISTER(mid, gaofs::rpc::tag::remove_metadata, rpc_rm_node_in_t,
                   rpc_rm_metadata_out_t, rpc_srv_remove_metadata);
    MARGO_REGISTER(mid, gaofs::rpc::tag::remove_data, rpc_rm_node_in_t,
                   rpc_err_out_t, rpc_srv_remove_data);
    MARGO_REGISTER(mid, gaofs::rpc::tag::update_metadentry,
                   rpc_update_metadentry_in_t, rpc_err_out_t,
                   rpc_srv_update_metadentry);
    MARGO_REGISTER(mid, gaofs::rpc::tag::get_metadentry_size, rpc_path_only_in_t,
                   rpc_get_metadentry_size_out_t, rpc_srv_get_metadentry_size);
    MARGO_REGISTER(mid, gaofs::rpc::tag::update_metadentry_size,
                   rpc_update_metadentry_size_in_t,
                   rpc_update_metadentry_size_out_t,
                   rpc_srv_update_metadentry_size);
    MARGO_REGISTER(mid, gaofs::rpc::tag::get_dirents, rpc_get_dirents_in_t,
                   rpc_get_dirents_out_t, rpc_srv_get_dirents);
    MARGO_REGISTER(mid, gaofs::rpc::tag::get_dirents_extended,
                   rpc_get_dirents_in_t, rpc_get_dirents_out_t,
                   rpc_srv_get_dirents_extended);
#ifdef HAS_SYMLINKS
    MARGO_REGISTER(mid, gaofs::rpc::tag::mk_symlink, rpc_mk_symlink_in_t,
                   rpc_err_out_t, rpc_srv_mk_symlink);
#endif
    MARGO_REGISTER(mid, gaofs::rpc::tag::write, rpc_write_data_in_t,
                   rpc_data_out_t, rpc_srv_write);
    MARGO_REGISTER(mid, gaofs::rpc::tag::read, rpc_read_data_in_t,
                   rpc_data_out_t, rpc_srv_read);
    MARGO_REGISTER(mid, gaofs::rpc::tag::truncate, rpc_trunc_in_t, rpc_err_out_t,
                   rpc_srv_truncate);
    MARGO_REGISTER(mid, gaofs::rpc::tag::get_chunk_stat, rpc_chunk_stat_in_t,
                   rpc_chunk_stat_out_t, rpc_srv_get_chunk_stat);
}

// 设置margo实例和监听addr
void init_rpc_server() {
    hg_addr_t addr_self = nullptr;
    hg_size_t addr_self_cstring_sz = 128; // 地址长度
    char addr_self_cstring[128];
    struct hg_init_info hg_options = HG_INIT_INFO_INITIALIZER;
    hg_options.auto_sm = GAOFS_DATA->use_auto_sm() ? HG_TRUE : HG_FALSE;
    hg_options.stats = HG_FALSE;
    if(gaofs::rpc::protocol::ofi_psm2 == GAOFS_DATA->rpc_protocol())
        hg_options.na_init_info.progress_mode = NA_NO_BLOCK;
    // 启动Margo(这也会在内部初始化Argobots和Mercury)，mid是margo instance id
    auto margo_config = fmt::format(
            R"({{ "use_progress_thread" : true, "rpc_thread_count" : {} }})",
            gaofs::config::rpc::daemon_handler_xstreams);
    struct margo_init_info args = {nullptr};
    args.json_config = margo_config.c_str();
    args.hg_init_info = &hg_options;
    auto* mid = margo_init_ext(GAOFS_DATA->bind_addr().c_str(),
                               MARGO_SERVER_MODE, &args);

    if(mid == MARGO_INSTANCE_NULL) {
        throw runtime_error("Failed to initialize the Margo RPC server");
    }

    // 找出这个服务器正在监听的地址(完成时必须释放)
    auto hret = margo_addr_self(mid, &addr_self);
    if(hret != HG_SUCCESS) {
        margo_finalize(mid);
        throw runtime_error("Failed to retrieve server RPC address");
    }

    // 将地址转换为cstring(带有\0终止符)
    hret = margo_addr_to_string(mid, addr_self_cstring, &addr_self_cstring_sz,
                                addr_self);
    if(hret != HG_SUCCESS) {
        margo_addr_free(mid, addr_self);
        margo_finalize(mid);
        throw runtime_error("Failed to convert server RPC address to string");
    }
    margo_addr_free(mid, addr_self);

    std::string addr_self_str(addr_self_cstring);
    RPC_DATA->self_addr_str(addr_self_str);

    GAOFS_DATA->spdlogger()->info("{}() Accepting RPCs on address {}", __func__,
                                 addr_self_cstring);

    // 将mid放入RPC_DATA
    RPC_DATA->server_rpc_mid(mid);
    // 注册rpc处理进程
    register_server_rpcs(mid);
}

// 初始化守护进程环境和设置他的子例程
// 这包括连接到KV存储，启动Argobots I/O执行流，初始化元数据和数据后端，以及启动RPC服务器。
// 这包括连接到KV存储，启动Argobots I/O执行流
// 初始化元数据和数据后端，以及启动RPC服务器。
void init_environment() {
    // 初始化元数据数据库
    auto metadata_path = fmt::format("{}/{}", GAOFS_DATA->metadir(),
                                     gaofs::config::metadata::dir);
    fs::create_directories(metadata_path);
    try {
        GAOFS_DATA->mdb(std::make_shared<gaofs::metadata::MetadataDB>(
                metadata_path, GAOFS_DATA->dbbackend()));
    } catch(const std::exception& e) {
        GAOFS_DATA->spdlogger()->error(
                "{}() Failed to initialize metadata DB: {}", __func__,
                e.what());
        throw;
    }

    // 初始化distributor，也就是用来hash的
    GAOFS_DATA->spdlogger()->debug("{}() Initializing Distributor ", __func__);
    try {
#ifdef GAOFS_USE_GUIDED_DISTRIBUTION
        auto distributor = std::make_shared<gaofs::rpc::GuidedDistributor>();
#else
        auto distributor = std::make_shared<gaofs::rpc::SimpleHashDistributor>();
#endif
        RPC_DATA->distributor(distributor);
    } catch(const std::exception& e) {
        GAOFS_DATA->spdlogger()->error(
                "{}() Failed to initialize Distributor: {}", __func__,
                e.what());
        throw;
    }

#ifdef GAOFS_ENABLE_FORWARDING
    GAOFS_DATA->spdlogger()->debug("{}() Enable I/O forwarding mode", __func__);
#endif

#ifdef GAOFS_ENABLE_AGIOS
    // Initialize AGIOS scheduler
    GAOFS_DATA->spdlogger()->debug("{}() Initializing AGIOS scheduler: '{}'",
                                  __func__, "/tmp/agios.conf");
    try {
        agios_initialize();
    } catch(const std::exception& e) {
        GAOFS_DATA->spdlogger()->error(
                "{}() Failed to initialize AGIOS scheduler: {}", __func__,
                e.what());
        throw;
    }
#endif

    // Initialize Stats
    if(GAOFS_DATA->enable_stats() || GAOFS_DATA->enable_chunkstats())
        GAOFS_DATA->stats(std::make_shared<gaofs::utils::Stats>(
                GAOFS_DATA->enable_chunkstats(), GAOFS_DATA->enable_prometheus(),
                GAOFS_DATA->stats_file(), GAOFS_DATA->prometheus_gateway()));

    // 初始化数据后端
    auto chunk_storage_path = fmt::format("{}/{}", GAOFS_DATA->rootdir(),
                                          gaofs::config::data::chunk_dir);
    GAOFS_DATA->spdlogger()->debug("{}() Initializing storage backend: '{}'",
                                  __func__, chunk_storage_path);
    fs::create_directories(chunk_storage_path);
    try {
        GAOFS_DATA->storage(std::make_shared<gaofs::data::ChunkStorage>(
                chunk_storage_path, gaofs::config::rpc::chunksize));
    } catch(const std::exception& e) {
        GAOFS_DATA->spdlogger()->error(
                "{}() Failed to initialize storage backend: {}", __func__,
                e.what());
        throw;
    }

    // 用init_rpc_server()初始化margo
    GAOFS_DATA->spdlogger()->debug("{}() Initializing RPC server: '{}'",
                                  __func__, GAOFS_DATA->bind_addr());
    try {
        init_rpc_server();
    } catch(const std::exception& e) {
        GAOFS_DATA->spdlogger()->error(
                "{}() Failed to initialize RPC server: {}", __func__, e.what());
        throw;
    }

    // 初始化Argobots ESs以驱动IO 使用init_io_tasklet_pool()
    try {
        GAOFS_DATA->spdlogger()->debug("{}() Initializing I/O pool", __func__);
        init_io_tasklet_pool();
    } catch(const std::exception& e) {
        GAOFS_DATA->spdlogger()->error(
                "{}() Failed to initialize Argobots pool for I/O: {}", __func__,
                e.what());
        throw;
    }

    // 设置元数据配置
    GAOFS_DATA->atime_state(gaofs::config::metadata::use_atime);
    GAOFS_DATA->mtime_state(gaofs::config::metadata::use_mtime);
    GAOFS_DATA->ctime_state(gaofs::config::metadata::use_ctime);
    GAOFS_DATA->link_cnt_state(gaofs::config::metadata::use_link_cnt);
    GAOFS_DATA->blocks_state(gaofs::config::metadata::use_blocks);

    // 为根目录创建元条目
    gaofs::metadata::Metadata root_md{S_IFDIR | S_IRWXU | S_IRWXG | S_IRWXO}; //初始化mode
    try {
        gaofs::metadata::create("/", root_md);
    } catch(const gaofs::db_exception::ExistsException& e) {
        // launched on existing directory which is no error
    } catch(const std::exception& e) {
        throw runtime_error("Failed to write root metadentry to KV store: "s +
                            e.what());
    }

    // 设置hostfile，让客户端知道这个主机上正在运行一个守护进程
    if(!GAOFS_DATA->hosts_file().empty()) {
        gaofs::utils::populate_hosts_file();
    }
    GAOFS_DATA->spdlogger()->info("Startup successful. Daemon is ready.");
}

// todo: agios
#ifdef GAOFS_ENABLE_AGIOS
/**
 * @brief Initialize the AGIOS scheduling library
 */
void
agios_initialize() {
    char configuration[] = "/tmp/agios.conf";

    if(!agios_init(NULL, NULL, configuration, 0)) {
        GAOFS_DATA->spdlogger()->error(
                "{}() Failed to initialize AGIOS scheduler: '{}'", __func__,
                configuration);

        agios_exit();

        throw;
    }
}
#endif

// 破坏守护程序环境并优雅地关闭所有子例程
void destroy_enviroment() {
    GAOFS_DATA->spdlogger()->debug("{}() Removing mount directory", __func__);
    std::error_code ecode;
    // 删除挂载目录下的所有东西
    fs::remove_all(GAOFS_DATA->mountdir(), ecode);
    GAOFS_DATA->spdlogger()->debug("{}() Freeing I/O executions streams",
                                  __func__);
    // 释放io流
    for(unsigned int i = 0; i < RPC_DATA->io_streams().size(); i++) {
        ABT_xstream_join(RPC_DATA->io_streams().at(i));
        ABT_xstream_free(&RPC_DATA->io_streams().at(i));
    }

    if(!GAOFS_DATA->hosts_file().empty()) {
        GAOFS_DATA->spdlogger()->debug("{}() Removing hosts file", __func__);
        try {
            gaofs::utils::destroy_hosts_file();
        } catch(const fs::filesystem_error& e) {
            GAOFS_DATA->spdlogger()->debug("{}() hosts file not found",
                                          __func__);
        }
    }

    if(RPC_DATA->server_rpc_mid() != nullptr) {
        GAOFS_DATA->spdlogger()->debug("{}() Finalizing margo RPC server",
                                      __func__);
        margo_finalize(RPC_DATA->server_rpc_mid());
    }

    GAOFS_DATA->spdlogger()->info("{}() Closing metadata DB", __func__);
    GAOFS_DATA->close_mdb();


    // Delete rootdir/metadir if requested
    if(!keep_rootdir) {
        GAOFS_DATA->spdlogger()->info("{}() Removing rootdir and metadir ...",
                                     __func__);
        fs::remove_all(GAOFS_DATA->metadir(), ecode);
        fs::remove_all(GAOFS_DATA->rootdir(), ecode);
    }
    // statistics
    GAOFS_DATA->close_stats();
}

// 用于守护进程关闭信号处理的处理程序
void shutdown_handler(int dummy) {
    GAOFS_DATA->spdlogger()->info("{}() Received signal: '{}'", __func__,
                                 strsignal(dummy));
    shutdown_please.notify_all();
}

// 初始化守护进程日志记录环境
void initialize_loggers() {
    std::string path = gaofs::config::log::daemon_log_path;
    // Try to get log path from env variable
    std::string env_path_key = DAEMON_ENV_PREFIX;
    env_path_key += "LOG_PATH";
    char* env_path = getenv(env_path_key.c_str());
    if(env_path != nullptr) {
        path = env_path;
    }

    spdlog::level::level_enum level =
            gaofs::log::get_level(gaofs::config::log::daemon_log_level);
    // Try to get log path from env variable
    std::string env_level_key = DAEMON_ENV_PREFIX;
    env_level_key += "LOG_LEVEL";
    char* env_level = getenv(env_level_key.c_str());
    if(env_level != nullptr) {
        level = gaofs::log::get_level(env_level);
    }

    auto logger_names = std::vector<std::string>{
            "main",
            "MetadataModule",
            "DataModule",
    };

    gaofs::log::setup(logger_names, level, path);
}

// 解析来自用户的命令行参数
void parse_input(const cli_options& opts, const CLI::App& desc) {
    auto rpc_protocol = string(gaofs::rpc::protocol::ofi_sockets);
    if(desc.count("--rpc-protocol")) {
        rpc_protocol = opts.rpc_protocol;
        if(rpc_protocol != gaofs::rpc::protocol::ofi_verbs &&
           rpc_protocol != gaofs::rpc::protocol::ofi_sockets &&
           rpc_protocol != gaofs::rpc::protocol::ofi_psm2) {
            throw runtime_error(fmt::format(
                    "Given RPC protocol '{}' not supported. Check --help for supported protocols.",
                    rpc_protocol));
        }
    }

    auto use_auto_sm = desc.count("--auto-sm") != 0;
    GAOFS_DATA->use_auto_sm(use_auto_sm);
    GAOFS_DATA->spdlogger()->debug(
            "{}() Shared memory (auto_sm) for intra-node communication (IPCs) set to '{}'.",
            __func__, use_auto_sm);

    string addr{};
    if(desc.count("--listen")) {
        addr = opts.listen;
        // ofi+verbs requires an empty addr to bind to the ib interface
        // Ofi+verbs需要一个空addr绑定到ib接口
        if(rpc_protocol == gaofs::rpc::protocol::ofi_verbs) {
            /*
             * FI_VERBS_IFACE : The prefix or the full name of the network
             * interface associated with the verbs device (default: ib) Mercury
             * does not allow to bind to an address when ofi+verbs is used
             */
            if(!secure_getenv("FI_VERBS_IFACE"))
                setenv("FI_VERBS_IFACE", addr.c_str(), 1);
            addr = ""s;
        }
    } else {
        if(rpc_protocol != gaofs::rpc::protocol::ofi_verbs)
            addr = gaofs::rpc::get_my_hostname(true);
    }

    GAOFS_DATA->rpc_protocol(rpc_protocol);
    GAOFS_DATA->bind_addr(fmt::format("{}://{}", rpc_protocol, addr));

    string hosts_file;
    if(desc.count("--hosts-file")) {
        hosts_file = opts.hosts_file;
    } else {
        hosts_file = gaofs::env::get_var(gaofs::env::HOSTS_FILE,
                                        gaofs::config::hostfile_path);
    }
    GAOFS_DATA->hosts_file(hosts_file);

    assert(desc.count("--mountdir"));
    auto mountdir = opts.mountdir;
    // Create mountdir. We use this dir to get some information on the
    // underlying fs with statfs in gaofs_statfs
    // 创建mountdir。我们使用这个dir在gaofs_statfs中获取底层fs的一些信息
    fs::create_directories(mountdir);
    GAOFS_DATA->mountdir(fs::canonical(mountdir).native());

    assert(desc.count("--rootdir"));
    auto rootdir = opts.rootdir;

#ifdef GAOFS_ENABLE_FORWARDING
    // In forwarding mode, the backend is shared
    auto rootdir_path = fs::path(rootdir);
#else
    auto rootdir_path = fs::path(rootdir);
    if(desc.count("--rootdir-suffix")) {
        if(opts.rootdir_suffix == gaofs::config::data::chunk_dir ||
           opts.rootdir_suffix == gaofs::config::metadata::dir)
            throw runtime_error(fmt::format(
                    "rootdir_suffix '{}' is reserved and not allowed.",
                    opts.rootdir_suffix));
        if(opts.rootdir_suffix.find('#') != string::npos)
            throw runtime_error(fmt::format(
                    "The character '#' in the rootdir_suffix is reserved and not allowed."));
        // append path with a directory separator
        rootdir_path /= opts.rootdir_suffix;
        GAOFS_DATA->rootdir_suffix(opts.rootdir_suffix);
    }
#endif

    if(desc.count("--clean-rootdir")) {
        // may throw exception (caught in main)
        GAOFS_DATA->spdlogger()->debug("{}() Cleaning rootdir '{}' ...",
                                      __func__, rootdir_path.native());
        fs::remove_all(rootdir_path.native());
        GAOFS_DATA->spdlogger()->info("{}() rootdir cleaned.", __func__);
    }

    if(desc.count("--clean-rootdir-finish")) {
        keep_rootdir = false;
    }

    GAOFS_DATA->spdlogger()->debug("{}() Root directory: '{}'", __func__,
                                  rootdir_path.native());
    fs::create_directories(rootdir_path);
    GAOFS_DATA->rootdir(rootdir_path.native());

    if(desc.count("--metadir")) {
        auto metadir = opts.metadir;

#ifdef GAOFS_ENABLE_FORWARDING
        auto metadir_path = fs::path(metadir) / fmt::format_int(getpid()).str();
#else
        auto metadir_path = fs::path(metadir);
#endif
        if(desc.count("--clean-rootdir")) {
            // may throw exception (caught in main)
            GAOFS_DATA->spdlogger()->debug("{}() Cleaning metadir '{}' ...",
                                          __func__, metadir_path.native());
            fs::remove_all(metadir_path.native());
            GAOFS_DATA->spdlogger()->info("{}() metadir cleaned.", __func__);
        }
        fs::create_directories(metadir_path);
        GAOFS_DATA->metadir(fs::canonical(metadir_path).native());

        GAOFS_DATA->spdlogger()->debug("{}() Meta directory: '{}'", __func__,
                                      metadir_path.native());
    } else {
        // use rootdir as metadata dir
        auto metadir = opts.rootdir;

#ifdef GAOFS_ENABLE_FORWARDING
        auto metadir_path = fs::path(metadir) / fmt::format_int(getpid()).str();
        fs::create_directories(metadir_path);
        GAOFS_DATA->metadir(fs::canonical(metadir_path).native());
#else
        GAOFS_DATA->metadir(GAOFS_DATA->rootdir());
#endif
    }

    // 设置元数据数据库类型，默认是rocksdb
    if(desc.count("--dbbackend")) {
        if(opts.dbbackend == gaofs::metadata::rocksdb_backend ||
           opts.dbbackend == gaofs::metadata::parallax_backend) {
#ifndef GAOFS_ENABLE_PARALLAX
            if(opts.dbbackend == gaofs::metadata::parallax_backend) {
                throw runtime_error(fmt::format(
                        "dbbackend '{}' was not compiled and is disabled. "
                        "Pass -DGAOFS_ENABLE_PARALLAX:BOOL=ON to CMake to enable.",
                        opts.dbbackend));
            }
#endif
#ifndef GAOFS_ENABLE_ROCKSDB
            if(opts.dbbackend == gaofs::metadata::rocksdb_backend) {
                throw runtime_error(fmt::format(
                        "dbbackend '{}' was not compiled and is disabled. "
                        "Pass -DGAOFS_ENABLE_ROCKSDB:BOOL=ON to CMake to enable.",
                        opts.dbbackend));
            }
#endif
            GAOFS_DATA->dbbackend(opts.dbbackend);
        } else {
            throw runtime_error(
                    fmt::format("dbbackend '{}' is not valid. Consult `--help`",
                                opts.dbbackend));
        }

    } else
        GAOFS_DATA->dbbackend(gaofs::metadata::rocksdb_backend);

    if(desc.count("--parallaxsize")) { // Size in GB
        GAOFS_DATA->parallax_size_md(stoi(opts.parallax_size));
    }

    /*
     * Statistics collection arguments
     */
    if(desc.count("--enable-collection")) {
        GAOFS_DATA->enable_stats(true);
        GAOFS_DATA->spdlogger()->info("{}() Statistic collection enabled",
                                     __func__);
    }
    if(desc.count("--enable-chunkstats")) {
        GAOFS_DATA->enable_chunkstats(true);
        GAOFS_DATA->spdlogger()->info("{}() Chunk statistics collection enabled",
                                     __func__);
    }


// todo prometheus
#ifdef GAOFS_ENABLE_PROMETHEUS
    if(desc.count("--enable-prometheus")) {
        GAOFS_DATA->enable_prometheus(true);
        if(GAOFS_DATA->enable_stats() || GAOFS_DATA->enable_chunkstats())
            GAOFS_DATA->spdlogger()->info(
                    "{}() Statistics output to Prometheus enabled", __func__);
        else
            GAOFS_DATA->spdlogger()->warn(
                    "{}() Prometheus statistics output enabled but no stat collection is enabled. There will be no output to Prometheus",
                    __func__);
    }

    if(desc.count("--prometheus-gateway")) {
        auto gateway = opts.prometheus_gateway;
        GAOFS_DATA->prometheus_gateway(gateway);
        if(GAOFS_DATA->enable_prometheus())
            GAOFS_DATA->spdlogger()->info("{}() Prometheus gateway set to '{}'",
                                         __func__, gateway);
        else
            GAOFS_DATA->spdlogger()->warn(
                    "{}() Prometheus gateway was set but Prometheus is disabled.");
    }
#endif

    if(desc.count("--output-stats")) {
        auto stats_file = opts.stats_file;
        GAOFS_DATA->stats_file(stats_file);
        if(GAOFS_DATA->enable_stats() || GAOFS_DATA->enable_chunkstats())
            GAOFS_DATA->spdlogger()->info(
                    "{}() Statistics are written to file '{}'", __func__,
                    stats_file);
        else
            GAOFS_DATA->spdlogger()->warn(
                    "{}() --output-stats argument used but no stat collection is enabled. There will be no output to file '{}'",
                    __func__, stats_file);
    } else {
        GAOFS_DATA->stats_file("");
        GAOFS_DATA->spdlogger()->debug("{}() Statistics output disabled",
                                      __func__);
    }
}

// 启动所有子程序并等待条件变量将其关闭。Daemon会对以下信号做出反应:
int main(int argc, const char* argv[]) {
    CLI::App desc{"Allowed options"};
    cli_options opts{};
    // clang-format off
    desc.add_option("--mountdir,-m", opts.mountdir,
                    "Virtual mounting directory where GekkoFS is available.")
            ->required();
    desc.add_option(
                    "--rootdir,-r", opts.rootdir,
                    "Local data directory where GekkoFS data for this daemon is stored.")
            ->required();
    desc.add_option(
            "--rootdir-suffix,-s", opts.rootdir_suffix,
            "Creates an additional directory within the rootdir, allowing multiple daemons on one node.");
    desc.add_option(
            "--metadir,-i", opts.metadir,
            "Metadata directory where GekkoFS RocksDB data directory is located. If not set, rootdir is used.");
    desc.add_option(
            "--listen,-l", opts.listen,
            "Address or interface to bind the daemon to. Default: local hostname.\n"
            "When used with ofi+verbs the FI_VERBS_IFACE environment variable is set accordingly "
            "which associates the verbs device with the network interface. In case FI_VERBS_IFACE "
            "is already defined, the argument is ignored. Default 'ib'.");
    desc.add_option("--hosts-file,-H", opts.hosts_file,
                    "Shared file used by deamons to register their "
                    "endpoints. (default './gaofs_hosts.txt')");
    desc.add_option(
            "--rpc-protocol,-P", opts.rpc_protocol,
            "Used RPC protocol for inter-node communication.\n"
            "Available: {ofi+sockets, ofi+verbs, ofi+psm2} for TCP, Infiniband, "
            "and Omni-Path, respectively. (Default ofi+sockets)\n"
            "Libfabric must have enabled support verbs or psm2.");
    desc.add_flag(
            "--auto-sm",
            "Enables intra-node communication (IPCs) via the `na+sm` (shared memory) protocol, "
            "instead of using the RPC protocol. (Default off)");
    desc.add_flag(
            "--clean-rootdir",
            "Cleans Rootdir >before< launching the deamon");
    desc.add_flag(
            "--clean-rootdir-finish,-c",
            "Cleans Rootdir >after< the deamon finishes");
    desc.add_option(
            "--dbbackend,-d", opts.dbbackend,
            "Metadata database backend to use. Available: {rocksdb, parallaxdb}\n"
            "RocksDB is default if not set. Parallax support is experimental.\n"
            "Note, parallaxdb creates a file called rocksdbx with 8GB created in metadir.");
    desc.add_option("--parallaxsize", opts.parallax_size,
                    "parallaxdb - metadata file size in GB (default 8GB), "
                    "used only with new files");
    desc.add_flag(
            "--enable-collection",
            "Enables collection of general statistics. "
            "Output requires either the --output-stats or --enable-prometheus argument.");
    desc.add_flag(
            "--enable-chunkstats",
            "Enables collection of data chunk statistics in I/O operations."
            "Output requires either the --output-stats or --enable-prometheus argument.");
    desc.add_option(
            "--output-stats", opts.stats_file,
            "Creates a thread that outputs the server stats each 10s to the specified file.");
#ifdef GAOFS_ENABLE_PROMETHEUS
    desc.add_flag(
                "--enable-prometheus",
                "Enables prometheus output and a corresponding thread.");

    desc.add_option(
                "--prometheus-gateway", opts.prometheus_gateway,
                "Defines the prometheus gateway <ip:port> (Default 127.0.0.1:9091).");
#endif

    desc.add_flag("--version", "Print version and exit.");
    // clang-format on
    try {
        desc.parse(argc, argv);
    } catch(const CLI::ParseError& e) {
        return desc.exit(e);
    }

    // 输出统计信息
    if(desc.count("--version")) {
        cout << GAOFS_VERSION_STRING << endl;
#ifndef NDEBUG
        cout << "Debug: ON" << endl;
#else
        cout << "Debug: OFF" << endl;
#endif
#ifdef CREATE_CHECK_PARENTS
        cout << "Create check parents: ON" << endl;
#else
        cout << "Create check parents: OFF" << endl;
#endif
        cout << "Chunk size: " << gaofs::config::rpc::chunksize << " bytes"
             << endl;
        return EXIT_SUCCESS;
    }
    // 初始化日志
    initialize_loggers();
    GAOFS_DATA->spdlogger(spdlog::get("main"));

    // 解析命令行输入
    try {
        parse_input(opts, desc);
    } catch(const std::exception& e) {
        cerr << fmt::format("Parsing arguments failed: '{}'. Exiting.",
                            e.what());
        return EXIT_FAILURE;
    }

    /*
     * 初始化环境
     */
    try {
        GAOFS_DATA->spdlogger()->info("{}() Initializing environment", __func__);
        init_environment();
    } catch(const std::exception& e) {
        auto emsg =
                fmt::format("Failed to initialize environment: {}", e.what());
        GAOFS_DATA->spdlogger()->error(emsg);
        cerr << emsg << endl;
        destroy_enviroment();
        return EXIT_FAILURE;
    }

    signal(SIGINT, shutdown_handler);
    signal(SIGTERM, shutdown_handler);
    signal(SIGKILL, shutdown_handler);

    unique_lock<mutex> lk(mtx);
    // 等待关闭信号
    shutdown_please.wait(lk);
    GAOFS_DATA->spdlogger()->info("{}() Shutting down...", __func__);
    destroy_enviroment();
    GAOFS_DATA->spdlogger()->info("{}() Complete. Exiting...", __func__);
    return EXIT_SUCCESS;
}

/*
int main() {
    init_environment_test();
    std::cout << "test over" << std::endl;
    return 0;
}
 */

