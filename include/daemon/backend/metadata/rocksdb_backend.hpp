//
// Created by DELL on 2022/12/31.
//

#ifndef GAOFS_ROCKSDB_BACKEND_HPP
#define GAOFS_ROCKSDB_BACKEND_HPP

#include <memory>
#include <spdlog/spdlog.h>
#include <daemon/backend/exceptions.hpp>
#include <tuple>
#include <daemon/backend/metadata/metadata_backend.hpp>
#include <rocksdb/db.h>
namespace rdb = rocksdb;

namespace gaofs::metadata {

class RocksDBBackend : public MetadataBackend<RocksDBBackend> {

private:
    std::unique_ptr<rdb::DB> db_;
    rdb::Options options_;
    rdb::WriteOptions write_opts_;

public:
    // 构造函数
    explicit RocksDBBackend(const std::string& path);
    // 析构函数
    virtual ~RocksDBBackend();

    /*
     * 根据db的状态抛出异常
     * @param RocksDB status
     * @throws DBException
     */
    static inline void throw_status_excpt(const rdb::Status& s);

    void optimize_database_impl();

    // metadata部分
    std::string get_impl(const std::string& key) const;

    void put_impl(const std::string& key, const std::string& val);

    void put_no_exist_impl(const std::string& key, const std::string& val);

    void remove_impl(const std::string& key);

    bool exists_impl(const std::string& key);

    void update_impl(const std::string& old_key, const std::string& new_key, const std::string& val);

    void increase_size_impl(const std::string& key, size_t size, bool append);

    void decrease_size_impl(const std::string& key, size_t size);

    std::vector<std::pair<std::string, bool>> get_dirents_impl(const std::string& dir) const;

    std::vector<std::tuple<std::string, bool, size_t, time_t>> get_dirents_extended_impl(const std::string& dir) const;

    //firstchunk部分
    std::string get_first_chunk_impl(const std::string& key) const;

    void put_first_chunk_impl(const std::string& key, const std::string& val);

    void put_no_exist_first_chunk_impl(const std::string& key, const std::string& val);

    void remove_first_chunk_impl(const std::string& key);

    bool exists_first_chunk_impl(const std::string& key);

    void update_first_chunk_impl(const std::string& old_key, const std::string& new_key, const std::string& val);

    // 调试部分
    void iterate_all_impl() const;
};

} // namespace gaofs::metadata

#endif //GAOFS_ROCKSDB_BACKEND_HPP
