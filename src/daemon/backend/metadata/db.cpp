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
            //GAOFS_METADATA_MOD->log()->trace("Using RocksDB directory '{}'", metadata_path);
            return std::make_unique<RocksDBBackend>(metadata_path);
#endif
        } // TODO: 如果要使用其他的backend的，要添加分支

        //GAOFS_METADATA_MOD->log()->error("No valid metadata backend selected");
        exit(EXIT_FAILURE);
    }
};

MetadataDB::MetadataDB(const std::string &path, const std::string_view database) : path_(path) {
    // 设置日志
    // GAOFS_METADATA_MOD->log(spdlog::get(GAOFS_METADATA_MOD->LOGGER_NAME));
    // assert(GAOFS_METADATA_MOD->log());
    // log_ = spdlog::get(GAOFS_METADATA_MOD->LOGGER_NAME);
    // assert(log_);

    // 设置backend
    backend_ = MetadataDBFactory::create(path, database);
}

MetadataDB::~MetadataDB() {
    backend_.reset();
}

std::string MetadataDB::get(const std::string &key) const {
    auto key_suffix = key;
    key_suffix.append("_m");
    return backend_->get(key_suffix);
}

void MetadataDB::put(const std::string &key, const std::string &val) {
    // key要是绝对路径且末尾没有/ 根目录除外
    assert(gaofs::path::is_absolute(key));
    assert(key == "/" || !gaofs::path::has_trailing_slash(key));

    auto key_suffix = key;
    key_suffix.append("_m");
    backend_->put(key_suffix, val);
}

void MetadataDB::put_no_exist(const std::string &key, const std::string &val) {
    assert(gaofs::path::is_absolute(key));
    assert(key == "/" || !gaofs::path::has_trailing_slash(key));

    auto key_suffix = key;
    key_suffix.append("_m");
    backend_->put_no_exist(key_suffix, val);
}

void MetadataDB::remove(const std::string &key) {
    auto key_suffix = key;
    key_suffix.append("_m");
    backend_->remove(key_suffix);
}

bool MetadataDB::exists(const std::string &key) {
    auto key_suffix = key;
    key_suffix.append("_m");
    return backend_->exists(key_suffix);
}

void MetadataDB::increase_size(const std::string &key, size_t size, bool append) {
    auto key_suffix = key;
    key_suffix.append("_m");
    backend_->increase_size(key_suffix, size, append);
}

void MetadataDB::decrease_size(const std::string &key, size_t size) {
    auto key_suffix = key;
    key_suffix.append("_m");
    backend_->decrease_size(key_suffix, size);
}

void MetadataDB::update(const std::string &old_key, const std::string &new_key, const std::string &val) {
    auto old_key_suffix = old_key;
    old_key_suffix.append("_m");

    auto new_key_suffix = new_key;
    new_key_suffix.append("_m");
    backend_->update(old_key_suffix, new_key_suffix, val);
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

    return backend_->get_dirents_extended(root_path);
}

std::string MetadataDB::get_first_chunk(const std::string &key) {
    auto key_suffix = key;
    key_suffix.append("_f");
    return backend_->get_first_chunk(key_suffix);
}

void MetadataDB::put_first_chunk(const std::string &key, const std::string &val) {
    // key要是绝对路径且末尾没有/ 根目录除外
    assert(gaofs::path::is_absolute(key));
    assert(key == "/" || !gaofs::path::has_trailing_slash(key));

    auto key_suffix = key;
    key_suffix.append("_f");
    backend_->put_first_chunk(key_suffix, val);
}

void MetadataDB::put_no_exist_first_chunk(const std::string &key, const std::string &val) {
    assert(gaofs::path::is_absolute(key));
    assert(key == "/" || !gaofs::path::has_trailing_slash(key));

    auto key_suffix = key;
    key_suffix.append("_f");
    backend_->put_no_exist_first_chunk(key_suffix, val);
}

void MetadataDB::remove_first_chunk(const std::string &key) {
    auto key_suffix = key;
    key_suffix.append("_f");
    backend_->remove_first_chunk(key_suffix);
}

bool MetadataDB::exists_first_chunk(const std::string &key) {
    auto key_suffix = key;
    key_suffix.append("_f");
    return backend_->exists_first_chunk(key_suffix);
}

void MetadataDB::update_first_chunk(const std::string &old_key, const std::string &new_key, const std::string &val) {
    auto old_key_suffix = old_key;
    old_key_suffix.append("_f");

    auto new_key_suffix = new_key;
    new_key_suffix.append("_f");
    backend_->update_first_chunk(old_key_suffix, new_key_suffix, val);
}

void MetadataDB::iterate_all() const {
    backend_->iterate_all();
}

} // namespace gaofs::metadata

