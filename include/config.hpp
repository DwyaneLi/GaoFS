//
// Created by DELL on 2022/12/17.
//

#ifndef GAOFS_CONFIG_HPP
#define GAOFS_CONFIG_HPP

#define CLIENT_ENV_PREFIX "LIBGAOFS_"
#define DAEMON_ENV_PREFIX "GAOFS_DAEMON_"
#define COMMON_ENV_PREFIX "GAOFS_"

namespace gaofs::config {
constexpr auto hostfile_path = "./gaofs_hosts.txt";



namespace io {
constexpr auto zero_buffer_before_read = false;
} // namespace io




namespace log {
constexpr auto client_log_path = "/tmp/gaofs_client.log";
constexpr auto client_log_level = "info,errors,critical,hermes";

constexpr auto daemon_log_path = "/tmp/gkfs_daemon.log";
constexpr auto daemon_log_level = 4; // info

} // namespace log



namespace metadata {
constexpr auto dir = "metadata";

// metadata处用的设置
constexpr auto use_atime = false;
constexpr auto use_ctime = false;
constexpr auto use_mtime = false;
constexpr auto use_link_cnt = false;
constexpr auto use_blocks = false;

// 创建元数据时检查是否已经存在
constexpr auto create_exist_check = true;

// 是否隐式删除data
constexpr auto implicit_data_removal = true;
} // namespace gaofs::config::metadata


namespace data {
// 数据块文件存放的路径
constexpr auto chunk_dir = "chunks";
}


namespace rpc {
constexpr auto chunksize = 524288; //chunk大小, 512kB
constexpr auto dirents_buff_size = (8 * 1024 * 1024);

// 表示驱动块文件与本地文件系统之间I/O操作的并发进度数。该值直接映射到已创建的Argobots xstreams，
// 通过abt_snozer调度程序控制在单个池中
constexpr auto daemon_io_xstreams = 8;
// 守护进程中用于RPC处理程序的线程数
constexpr auto daemon_handler_xstreams = 4;
} // namespace gaofs::config::rpc



namespace rocksdb {
// rocksbd是否使用预写日志
constexpr  auto use_write_ahead_log = false;
} // namespace gaofs::config::rocksdb


namespace stats {
    constexpr auto max_stats = 1000000; ///< How many stats will be stored
    constexpr auto prometheus_gateway = "127.0.0.1:9091";
} // namespace stats


} // namespace gaofs::config

#endif //GAOFS_CONFIG_HPP
