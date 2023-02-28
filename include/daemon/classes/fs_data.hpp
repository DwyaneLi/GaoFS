//
// Created by DELL on 2023/1/16.
//

#ifndef GAOFS_FS_DATA_HPP
#define GAOFS_FS_DATA_HPP

#include <daemon/backend/metadata/db.hpp>
#include <daemon/backend/data/chunk_storage.hpp>

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

    // TODO: RPC management
    std::string hosts_file_{}; // 通信地址文档

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

    // TODO: Statistics

    // TODO: Prometheus

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

    // TODO: RPC management
    const std::string& hosts_file() const;

    void hosts_file(const std::string& hosts_file);

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

    // TODO: Parallax

    // TODO: Statistics

    // TODO: Prometheus


};

} // namespace gaofs::daemon

} // namespace gaofs



#endif //GAOFS_FS_DATA_HPP
