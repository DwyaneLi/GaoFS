//
// Created by DELL on 2023/1/16.
//

#ifndef GAOFS_FS_DATA_HPP
#define GAOFS_FS_DATA_HPP

#include <daemon/backend/metadata/db.hpp>
#include <daemon/backend/data/chunk_storage.hpp>

#include <common/statistics/stats.hpp>

#include <unordered_map>
#include <map>
#include <functional>
#include <string_view>

#include <spdlog/spdlog.h>
namespace gaofs {

namespace daemon {

class FsData {

private:
    FsData() = default;
    // logger
    std::shared_ptr<spdlog::logger> spdlogger_;

    // path
    std::string rootdir_{};
    std::string rootdir_suffix_{};
    std::string mountdir_{};
    std::string metadir_{};

    // RPC management
    std::string hosts_file_{}; // 通信地址文档
    std::string bind_addr_{};
    std::string rpc_protocol_{};
    bool use_auto_sm_;

    // Database
    std::shared_ptr<gaofs::metadata::MetadataDB> mdb_;
    std::string dbbackend_;

    // TODO: Parallax

    // Storage backend
    std::shared_ptr<gaofs::data::ChunkStorage> storage_;

    // configuable metadata
    bool atime_state_;
    bool mtime_state_;
    bool ctime_state_;
    bool link_cnt_state_;
    bool blocks_state_;

    // Statistics
    std::shared_ptr<gaofs::utils::Stats> stats_;
    bool enable_stats_ = false;
    bool enable_chunkstats_ = false;
    bool enable_prometheus_ = false;
    std::string stats_file_;

    // Prometheus
    std::string prometheus_gateway_ = gaofs::config::stats::prometheus_gateway;
public:

    static FsData* getInstance() {
        static FsData instance;
        return &instance;
    }

    FsData(FsData const&) = delete;

    void operator=(FsData const&) = delete;

    // getter and setter
    const std::shared_ptr<spdlog::logger>& spdlogger() const;

    void spdlogger(const std::shared_ptr<spdlog::logger>& spdlogger);

    const std::string& rootdir() const;

    void rootdir(const std::string& rootdir);

    const std::string& rootdir_suffix() const;

    void rootdir_suffix(const std::string& rootdir_suffix);

    const std::string& mountdir() const;

    void mountdir(const std::string& mountdir);

    const std::string& metadir() const;

    void metadir(const std::string& metadir);

    std::string_view dbbackend() const;

    void dbbackend(const std::string& dbbackend);

    const std::shared_ptr<gaofs::metadata::MetadataDB>& mdb() const;

    void mdb(const std::shared_ptr<gaofs::metadata::MetadataDB>& mdb);

    void close_mdb();

    //RPC management
    const std::string& hosts_file() const;

    void hosts_file(const std::string& hosts_file);

    const std::string& rpc_protocol() const;

    void rpc_protocol(const std::string& rpc_protocol);

    const std::string& bind_addr() const;

    void bind_addr(const std::string& addr);

    bool use_auto_sm() const;

    void use_auto_sm(bool use_auto_sm);

    bool atime_state() const;

    void atime_state(bool atime_state);

    bool mtime_state() const;

    void mtime_state(bool mtime_state);

    bool ctime_state() const;

    void ctime_state(bool ctime_state);

    bool link_cnt_state() const;

    void link_cnt_state(bool link_cnt_state);

    bool blocks_state() const;

    void blocks_state(bool blocks_state);

    // storage backend
    const std::shared_ptr<gaofs::data::ChunkStorage>& storage() const;

    void storage(const std::shared_ptr<gaofs::data::ChunkStorage>& storage);

    // Parallax
    unsigned long long parallax_size_md_ = 8589934592ull;

    // Statistics
    const std::shared_ptr<gaofs::utils::Stats>& stats() const;

    void stats(const std::shared_ptr<gaofs::utils::Stats>& stats);

    void close_stats();

    bool enable_stats() const;

    void enable_stats(bool enable_stats);

    bool enable_chunkstats() const;

    void enable_chunkstats(bool enable_chunkstats);

    const std::string& stats_file() const;

    void stats_file(const std::string& stats_file);

    // Prometheus
    bool enable_prometheus() const;

    void enable_prometheus(bool enable_prometheus);

    const std::string& prometheus_gateway() const;

    void prometheus_gateway(const std::string& prometheus_gateway_);

    //parallax
    unsigned long long parallax_size_md() const;

    void parallax_size_md(unsigned int size_md);

};

} // namespace gaofs::daemon

} // namespace gaofs



#endif //GAOFS_FS_DATA_HPP
