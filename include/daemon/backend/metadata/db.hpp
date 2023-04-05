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
// TODO: 如果要用其他的backend，在这里添加头文件
#ifdef GAOFS_ENABLE_ROCKSDB
#include <daemon/backend/metadata/rocksdb_backend.hpp>
#endif

namespace gaofs::metadata {

constexpr auto rocksdb_backend = "rocksdb";
// no use
constexpr auto parallax_backend = "parallaxdb";

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
    // metadata部分
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

    // firstchunk部分
    [[nodiscard]] std::string get_first_chunk(const std::string& key);

    void put_first_chunk(const std::string& key, const std::string& val);

    void put_no_exist_first_chunk(const std::string& key, const std::string& val);

    void remove_first_chunk(const std::string& key);

    bool exists_first_chunk(const std::string& key);

    void update_first_chunk(const std::string& old_key, const std::string& new_key, const std::string& val);
    // 调试函数，遍历
    void iterate_all() const;

};

} // namespace gaofs::metadata

#endif //GAOFS_DB_HPP
