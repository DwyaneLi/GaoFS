//
// Created by DELL on 2023/1/16.
//

#include <daemon/classes/fs_data.hpp>

namespace gaofs::daemon {

// getter and setter

const std::shared_ptr<spdlog::logger>& FsData::spdlogger() const {
    return spdlogger_;
}

void FsData::spdlogger(const std::shared_ptr<spdlog::logger>& spdlogger) {
    spdlogger_ = spdlogger;
}

const std::shared_ptr<gaofs::metadata::MetadataDB> & FsData::mdb() const {
    return mdb_;
}

void FsData::mdb(const std::shared_ptr<gaofs::metadata::MetadataDB> &mdb) {
    mdb_ = mdb;
}

void FsData::close_mdb() {
    mdb_.reset();
}

const std::string& FsData::rootdir() const {
    return rootdir_;
}

void FsData::rootdir(const std::string &rootdir) {
    rootdir_ = rootdir;
}

const std::string& FsData::rootdir_suffix() const {
    return rootdir_suffix_;
}

void FsData::rootdir_suffix(const std::string &rootdir_suffix) {
    rootdir_suffix_ = rootdir_suffix;
}

const std::string& FsData::mountdir() const {
    return mountdir_;
}

void FsData::mountdir(const std::string &mountdir) {
    mountdir_ = mountdir;
}

const std::string& FsData::metadir() const {
    return metadir_;
}

void FsData::metadir(const std::string &metadir) {
    metadir_ = metadir;
}

std::string_view FsData::dbbackend() const {
    return dbbackend_;
}

void FsData::dbbackend(const std::string &dbbackend) {
    dbbackend_ = dbbackend;
}

bool FsData::atime_state() const {
    return atime_state_;
}

void FsData::atime_state(bool atime_state) {
    atime_state_ = atime_state;
}

bool FsData::mtime_state() const {
    return mtime_state_;
}

void FsData::mtime_state(bool mtime_state) {
    mtime_state_ = mtime_state;
}

bool FsData::ctime_state() const {
    return ctime_state_;
}

void FsData::ctime_state(bool ctime_state) {
    ctime_state_ = ctime_state;
}

bool FsData::link_cnt_state() const {
    return link_cnt_state_;
}

void FsData::link_cnt_state(bool link_cnt_state) {
    link_cnt_state_ = link_cnt_state;
}

bool FsData::blocks_state() const {
    return blocks_state_;
}

void FsData::blocks_state(bool blocks_state) {
    blocks_state_ = blocks_state;
}

// storage backend
const std::shared_ptr<gaofs::data::ChunkStorage>& FsData::storage() const {
    return storage_;
}

void FsData::storage(const std::shared_ptr<gaofs::data::ChunkStorage> &storage) {
    storage_ = storage;
}

// Parallax
unsigned long long FsData::parallax_size_md() const {
    return parallax_size_md_;
}

void FsData::parallax_size_md(unsigned int size_md) {
    FsData::parallax_size_md_ = static_cast<unsigned long long>(
            size_md * 1024ull * 1024ull * 1024ull);
}


// Statistics
const std::shared_ptr<gaofs::utils::Stats>& FsData::stats() const {
    return stats_;
}

void FsData::stats(const std::shared_ptr<gaofs::utils::Stats>& stats) {
    FsData::stats_ = stats;
}

void FsData::close_stats() {
    stats_.reset();
}

bool FsData::enable_stats() const {
    return enable_stats_;
}

void FsData::enable_stats(bool enable_stats) {
    FsData::enable_stats_ = enable_stats;
}

bool FsData::enable_chunkstats() const {
    return enable_chunkstats_;
}

void FsData::enable_chunkstats(bool enable_chunkstats) {
    FsData::enable_chunkstats_ = enable_chunkstats;
}

const std::string& FsData::stats_file() const {
    return stats_file_;
}

void FsData::stats_file(const std::string& stats_file) {
    FsData::stats_file_ = stats_file;
}

// Prometheus
bool FsData::enable_prometheus() const {
    return enable_prometheus_;
}

const std::string& FsData::prometheus_gateway() const {
    return prometheus_gateway_;
}

void FsData::prometheus_gateway(const std::string& prometheus_gateway) {
    FsData::prometheus_gateway_ = prometheus_gateway;
}

void FsData::enable_prometheus(bool enable_prometheus) {
    FsData::enable_prometheus_ = enable_prometheus;
}

// RPC management
const std::string& FsData::hosts_file() const {
    return hosts_file_;
}

void FsData::hosts_file(const std::string &hosts_file) {
    hosts_file_ = hosts_file;

}
const std::string& FsData::rpc_protocol() const {
    return rpc_protocol_;
}

void FsData::rpc_protocol(const std::string& rpc_protocol) {
    rpc_protocol_ = rpc_protocol;
}

const std::string& FsData::bind_addr() const {
    return bind_addr_;
}

void FsData::bind_addr(const std::string& addr) {
    bind_addr_ = addr;
}

bool FsData::use_auto_sm() const {
    return use_auto_sm_;
}

void FsData::use_auto_sm(bool use_auto_sm) {
    use_auto_sm_ = use_auto_sm;
}

} // namespace gaofs::daemon
