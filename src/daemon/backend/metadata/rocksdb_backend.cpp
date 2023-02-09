//
// Created by DELL on 2022/12/31.
//

#include <daemon/backend/metadata/rocksdb_backend.hpp>
#include <daemon/backend/exceptions.hpp>
#include <daemon/backend/metadata/metadata_module.hpp>
#include <daemon/backend/metadata/merge.hpp>

#include <common/metadata.hpp>
#include <common/path_util.hpp>

#include <iostream>
extern "C" {
#include <sys/stat.h>
}


namespace gaofs::metadata {

void RocksDBBackend::optimize_database_impl() {
    // Maximum number of successive merge operations on a key in the memtable.
    options_.max_successive_merges = 128;
}

RocksDBBackend::RocksDBBackend(const std::string &path) {
    // 优化RocksDB
    options_.IncreaseParallelism();
    options_.OptimizeLevelStyleCompaction();

    // 如果数据库还不存在，就创建
    // If true, the database will be created if it is missing.
    options_.create_if_missing = true;
    options_.merge_operator.reset(new MetadataMergeOperator);

    // 设置merge的操作数量
    optimize_database_impl();
    // 默认不使用预写日志
    write_opts_.disableWAL = !gaofs::config::rocksdb::use_write_ahead_log;

    //利用刚刚的设置来开启一个rocksdb实例
    rdb::DB* rdb_ptr = nullptr;
    auto s = rdb::DB::Open(options_, path, &rdb_ptr);
    if(!s.ok()) {
        throw std::runtime_error("Failed to open RocksDB: " + s.ToString());
    }

    this->db_.reset(rdb_ptr);
}

RocksDBBackend::~RocksDBBackend() {
    this->db_.reset();
}

// 该函数根据db状态抛出异常
void RocksDBBackend::throw_status_excpt(const rdb::Status &s) {
    assert(!s.ok());
    using namespace gaofs::db_exception;
    if(s.IsNotFound()) {
        throw NotFoundException(s.ToString());
    } else {
        throw DBException(s.ToString());
    }
}

// 根据key获得value
std::string RocksDBBackend::get_impl(const std::string &key) const {
    std::string val;

    auto s = db_->Get(rdb::ReadOptions(), key, &val);
    if(!s.ok()) {
        throw_status_excpt(s);
    }

    return val;
}

// put key-value键值对
void RocksDBBackend::put_impl(const std::string &key, const std::string &val) {
    auto cop = CreateOperand(val);
    auto s = db_->Merge(write_opts_, key, cop.seriaize());
    if(!s.ok()) {
        throw_status_excpt(s);
    }
}

void RocksDBBackend::put_no_exist_impl(const std::string &key, const std::string &val) {
    if(exists(key)) {
        throw gaofs::db_exception::ExistsException(key);
    }
    put(key, val);
}

void RocksDBBackend::remove_impl(const std::string &key) {
    auto s = db_->Delete(write_opts_, key);
    if(!s.ok()) {
        throw_status_excpt(s);
    }
}

bool RocksDBBackend::exists_impl(const std::string &key) {
    std::string val;

    auto s = db_->Get(rdb::ReadOptions(), key, &val);
    if(!s.ok()) {
        if(s.IsNotFound()) {
            return false;
        } else {
            throw_status_excpt(s);
        }
    }
    return true;
}

void RocksDBBackend::update_impl(const std::string &old_key, const std::string &new_key, const std::string &val) {
    rdb::WriteBatch batch;
    batch.Delete(old_key);
    batch.Put(new_key, val);
    auto s = db_->Write(write_opts_, &batch);
    if(!s.ok()) {
        throw_status_excpt(s);
    }
}

void RocksDBBackend::increase_size_impl(const std::string &key, size_t size, bool append) {
    auto iop = IncreaseSizeOperand(size, append);
    auto s = db_->Merge(write_opts_, key, iop.seriaize());
    if(!s.ok()) {
        throw_status_excpt(s);
    }
}

void RocksDBBackend::decrease_size_impl(const std::string &key, size_t size) {
    auto dop = DecreaseSizeOperand(size);
    auto s = db_->Merge(write_opts_, key, dop.seriaize());
    if(!s.ok()) {
        throw_status_excpt(s);
    }
}


std::vector<std::pair<std::string, bool>> RocksDBBackend::get_dirents_impl(const std::string &dir) const {
    std::vector<std::pair<std::string, bool>> entries;

    auto root_path = dir;
    rdb::ReadOptions readOptions;
    auto it = db_->NewIterator(readOptions);

    auto root_dir_key = "/_m";
    for(it->Seek(root_path); it->Valid() && it->key().starts_with(root_path) && it->key().ends_with("_m"); it->Next()) {
        if(it->key().starts_with(root_dir_key) && it->key().size() == 3) {
            // 跳过root
            continue;
        }

        // 获得文件名
        auto name = it->key().ToString();
        if(name.find_first_of('/', root_path.size()) != std::string::npos) {
            // 跳过root/xxx/xx的
            continue;
        }


        name = name.substr(root_path.size(), name.size() - root_path.size() - 2);

        assert(!name.empty());

        Metadata md(it->value().ToString());
        auto is_dir = S_ISDIR(md.mode());
        // std::move将左值转化成右值
        entries.emplace_back(std::move(name), is_dir);
    }
    assert(it->status().ok());
    return entries;
}


std::vector<std::tuple<std::string, bool, size_t, time_t>> RocksDBBackend::get_dirents_extended_impl(
        const std::string &dir) const {
    std::vector<std::tuple<std::string, bool, size_t, time_t>> entries;

    auto root_path = dir;
    rdb::ReadOptions readOptions;
    auto it = db_->NewIterator(readOptions);

    auto root_dir_key = "/_m";
    for(it->Seek(root_path); it->Valid() && it->key().starts_with(root_path) && it->key().ends_with({"_f"}); it->Next()) {
        if(it->key().starts_with(root_dir_key) && it->key().size() == 3) {
            // 跳过root本身
            continue;
        }

        // 获得文件名
        auto name = it->key().ToString();
        if(name.find_first_of('/', root_path.size()) != std::string::npos) {
            // 跳过root/xxx/xx的
            continue;
        }


        name = name.substr(root_path.size(), name.size() - root_path.size() - 2);

        assert(!name.empty());

        Metadata md(it->value().ToString());
        auto is_dir = S_ISDIR(md.mode());
        // std::move将左值转化成右值
        entries.emplace_back(std::forward_as_tuple(std::move(name), is_dir, md.size(), md.ctime()));
    }
    assert(it->status().ok());
    return entries;
}

std::string RocksDBBackend::get_first_chunk_impl(const std::string &key) const {
    std::string val;

    auto s = db_->Get(rdb::ReadOptions(), key, &val);
    if(!s.ok()) {
        throw_status_excpt(s);
    }

    return val;
}

void RocksDBBackend::put_first_chunk_impl(const std::string &key, const std::string &val) {
    auto s = db_->Put(write_opts_, key, val);
    if(!s.ok()) {
        throw_status_excpt(s);
    }
}

void RocksDBBackend::put_no_exist_first_chunk_impl(const std::string &key, const std::string &val) {
    if(exists(key)) {
        throw gaofs::db_exception::ExistsException(key);
    }
    put(key, val);
}

bool RocksDBBackend::exists_first_chunk_impl(const std::string &key) {
    std::string val;

    auto s = db_->Get(rdb::ReadOptions(), key, &val);
    if(!s.ok()) {
        if(s.IsNotFound()) {
            return false;
        } else {
            throw_status_excpt(s);
        }
    }
    return true;
}

void RocksDBBackend::remove_first_chunk_impl(const std::string &key) {
    auto s = db_->Delete(write_opts_, key);
    if(!s.ok()) {
        throw_status_excpt(s);
    }
}

void RocksDBBackend::update_first_chunk_impl(const std::string &old_key, const std::string &new_key,
                                             const std::string &val) {
    rdb::WriteBatch batch;
    batch.Delete(old_key);
    batch.Put(new_key, val);
    auto s = db_->Write(write_opts_, &batch);
    if(!s.ok()) {
        throw_status_excpt(s);
    }
}

void RocksDBBackend::iterate_all_impl() const {
    auto iter = db_->NewIterator(rdb::ReadOptions());
    for(iter->SeekToFirst(); iter->Valid(); iter->Next()) {
        std::cout << iter->key().ToString() << std::endl;
    }
}


} // namespace gaofs::metadata

