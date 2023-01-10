//
// Created by DELL on 2023/1/6.
//

#include <daemon/backend/metadata/db.hpp>
#include <daemon/backend/metadata/metadata_module.hpp>

#include <common/path_util.hpp>

#include <filesystem>

namespace fs = std::filesystem;

namespace gaofs::metadata {

struct MetadataDBFactory {
    static std::unique_ptr<AbstractMetadataBackend> create(const std::string& path, const std::string_view id) {
        if(id == gaofs::metadata::rocksdb_backend) {
#ifdef GAOFS_ENABLE_ROCKSDB
            auto metadata_path = fmt::format("{}/{}", path, gaofs::metadata::rocksdb_backend);
            fs::create_directories(metadata_path);
            GAOFS_METADATA_MOD->log()->trace("Using RocksDB directory '{}'", metadata_path);
            return std::make_unique<RocksDBBackend>(metadata_path);
#endif
        } // TODO: 如果要使用其他的backend的，要添加分支

        GAOFS_METADATA_MOD->log()->error("No valid metadata backend selected");
        exit(EXIT_FAILURE);
    }
};

MetadataDB::MetadataDB(const std::string &path, const std::string_view database) : path_(path) {
    // 设置backend
    backend_ = MetadataDBFactory::create(path, database);

    // 设置日志
    GAOFS_METADATA_MOD->log(spdlog::get(GAOFS_METADATA_MOD->LOGGER_NAME));
    assert(GAOFS_METADATA_MOD->log());
    log_ = spdlog::get(GAOFS_METADATA_MOD->LOGGER_NAME);
    assert(log_);
}

MetadataDB::~MetadataDB() {
    backend_.reset();
}

std::string MetadataDB::get(const std::string &key) const {
    return backend_->get(key);
}

void MetadataDB::put(const std::string &key, const std::string &val) {
    // key要是绝对路径且末尾没有/ 根目录除外
    assert(gaofs::path::is_absolute(key));
    assert(key == "/" || !gaofs::path::has_trailing_slash(key));

    backend_->put(key, val);
}

void MetadataDB::put_no_exist(const std::string &key, const std::string &val) {
    assert(gaofs::path::is_absolute(key));
    assert(key == "/" || !gaofs::path::has_trailing_slash(key));

    backend_->put_no_exist(key, val);
}

void MetadataDB::remove(const std::string &key) {
    backend_->remove(key);
}

bool MetadataDB::exists(const std::string &key) {
    backend_->exists(key);
}

void MetadataDB::increase_size(const std::string &key, size_t size, bool append) {
    backend_->increase_size(key, size, append);
}

void MetadataDB::decrease_size(const std::string &key, size_t size) {
    backend_->decrease_size(key, size);
}

void MetadataDB::update(const std::string &old_key, const std::string &new_key, const std::string &val) {
    backend_->update(old_key, new_key, val);
}

std::vector<std::pair<std::string, bool>> MetadataDB::get_dirents(const std::string &dir) const {
    auto root_path = dir;
    assert(gaofs::path::is_absolute(root_path));

    if(!gaofs::path::has_trailing_slash(root_path) && root_path.size() != 1) {
        // 给末尾没斜杠的非根路径加斜杠
        root_path.push_back('/');
    }

    return backend_->get_dirents(root_path);
}

std::vector<std::tuple<std::string, bool, size_t, time_t>> MetadataDB::get_dirents_extended(
        const std::string &dir) const {
    auto root_path = dir;
    assert(gaofs::path::is_absolute(root_path));

    if(!gaofs::path::has_trailing_slash(root_path) && root_path.size() != 1) {
        // 给末尾没斜杠的非根路径加斜杠
        root_path.push_back('/');
    }

    return backend_->get_dirents_extended(dir);
}


void MetadataDB::iterate_all() const {
    backend_->iterate_all();
}

} // namespace gaofs::metadata

