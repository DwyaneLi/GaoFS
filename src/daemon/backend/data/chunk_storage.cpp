//
// Created by DELL on 2023/2/15.
//
#include <daemon/backend/data/chunk_storage.hpp>
#include <daemon/backend/data/file_handle.hpp>
#include <daemon/backend/data/data_module.hpp>
#include <common/path_util.hpp>

#include <filesystem>
#include <spdlog/spdlog.h>

#include <cerrno>
extern "C" {
#include <sys/statfs.h>
#include <sys/stat.h>
#include <fcntl.h>
}

namespace fs = std::filesystem;
using namespace std;

namespace gaofs::data {

// 实现root_path_和 internal_path的拼接
std::string ChunkStorage::absolute(const std::string &internal_path) const {
    assert(gaofs::path::is_relative(internal_path));
    return fmt::format("{}/{}", root_path_, internal_path);
}

// 将目标文件的路径如：“/foo/bar” 替换成 “foo:bar”然后在本地文件系统里就是“/tmp/rootdir/<pid>/data/chunks/foo:bar”，
// 后面再加个文件序号就可以找到文件
std::string ChunkStorage::get_chunks_dir(const string &file_path) {
    assert(gaofs::path::is_absolute(file_path));
    string chunk_dir = file_path.substr(1);
    std::replace(chunk_dir.begin(), chunk_dir.end(), '/', ':');
    return chunk_dir;
}

// 根据给定的chunk id生成对应路径
std::string ChunkStorage::get_chunk_path(const string &file_path, gaofs::rpc::chnk_id_t chunk_id) {
    return fmt::format("{}/{}", get_chunks_dir(file_path), chunk_id);
}

// 生成放文件chunk的文件夹
void ChunkStorage::init_chunk_space(const string &file_path) const {
    auto chunk_dir = absolute(get_chunks_dir(file_path));
    auto err = mkdir(chunk_dir.c_str(), 0750);
    if(err == -1 && errno != EEXIST) {
        auto err_str = fmt::format(
                "{}() Failed to create chunk directory. File: '{}', Error: '{}'",
                __func__, file_path, errno
                );
        throw ChunkStorageException(errno, err_str);
    }
}

ChunkStorage::ChunkStorage(string &path, size_t chunksize) : root_path_(path), chunksize_(chunksize) {
    // 创建logger（日志）实例
    GAOFS_DATA_MOD->log(spdlog::get(GAOFS_DATA_MOD->LOGGER_NAME));
    assert(GAOFS_DATA_MOD->log());
    log_ = spdlog::get(GAOFS_DATA_MOD->LOGGER_NAME);
    assert(log_);
    assert(gaofs::path::is_absolute(root_path_));

    // 验证是否对放块的根目录有足够的访问权限
    if(access(root_path_.c_str(), W_OK | R_OK != 0)) {
        auto err_str = fmt::format(
                "{}() Insufficient permissions to create chunk directories in path '{}'",
                __func__, root_path_
                );
        // EPERM表明权限不够
        throw ChunkStorageException(EPERM, err_str);
    }
    log_->debug("{}() Chunk storage initialized with path: '{}'", __func__, root_path_);
}


} // namespace gaofs::data