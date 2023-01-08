//
// Created by DELL on 2023/1/6.
//

#ifndef GAOFS_DB_HPP
#define GAOFS_DB_HPP

#include <memory>
#include <spdlog/spdlog.h>
#include <daemon/backend/exceptions.hpp>
#include <tuple>
#include <daemon/backend/metadata/metadata_backend.hpp>

// 对应后端的头文件
#include <daemon/backend/metadata/rocksdb_backend.hpp>

namespace gaofs::metadata {

constexpr auto rocksdb_backend = "rocksb";

class MetadataDB {

private:
    std::string path_;
    std::shared_ptr<spdlog::logger> log_;
    std::unique_ptr<AbstractMetadataBackend> backend_;

public:
    // 构造函数
    MetadataDB(const std::string& path, const std::string_view database);

    ~MetadataDB();

    // 各类方法
    [[nodiscard]] std::string get(const std::string& key) const;

    void put(const std::string& key, const std::string& val);

    void put_no_exist(const std::string& key, const std::string& val);

    void remove(const std::string& key);

    bool exists(const std::string& key);

    void update(const std::string& old_key, const std::string& new_key, const std::string& val);

    void increase_size(const std::string& key, size_t size, bool append);

    void decrease_size(const std::string& key, size_t size);

    [[nodiscard]] std::vector<std::pair<std::string, bool>>
    get_dirents(const std::string& dir) const;

    [[nodiscard]] std::vector<std::tuple<std::string, bool, size_t, time_t>>
    get_dirents_extended(const std::string& dir) const;

    // 调试函数，遍历
    void iterate_all() const;

};

} // namespace gaofs::metadata

#endif //GAOFS_DB_HPP
