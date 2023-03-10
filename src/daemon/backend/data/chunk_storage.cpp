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

// 删除file对应的存chunk的目录
void ChunkStorage::destroy_chunk_space(const string &file_path) const {
    auto chunk_dir = absolute(get_chunks_dir(file_path));
    try {
        auto n = fs::remove_all(chunk_dir);
        log_->debug("{}() Removed '{}' files and directories from '{}'",
                    __func__, n, chunk_dir);
    } catch (const fs::filesystem_error& e) {
        auto err_str = fmt::format("{}() Failed to remove chunk directory. Path: '{}', Error: '{}'",
                                   __func__, chunk_dir, e.what());
        throw ChunkStorageException(e.code().value(), err_str);
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
    if(access(root_path_.c_str(), W_OK | R_OK) != 0) {
        auto err_str = fmt::format(
                "{}() Insufficient permissions to create chunk directories in path '{}'",
                __func__, root_path_
                );
        // EPERM表明权限不够
        throw ChunkStorageException(EPERM, err_str);
    }
    log_->debug("{}() Chunk storage initialized with path: '{}'", __func__, root_path_);
}

// 在本地文件系统上创造一个file_path对应的文件夹来保存对应的chunk， 并从buf中向文件中写入
ssize_t ChunkStorage::write_chunk(const string &file_path, gaofs::rpc::chnk_id_t chunk_id, const char *buf, size_t size,
                                  off64_t offset) const {
    assert((offset+size) <= chunksize_);
    // 初始化放chunk的文件夹，如果以及有了也无所谓
    init_chunk_space(file_path);
    // 获取全路径
    auto chunk_path = absolute(get_chunk_path(file_path, chunk_id));

    // 生成文件对应的file handle，并且用open函数打开，如果没有的话创建
    FileHandle fh(open(chunk_path.c_str(), O_WRONLY | O_CREAT, 0640), chunk_path);
    if(!fh.valid()) {
        auto err_str = fmt::format(
                "{}() Failed to open chunk file for write. File: '{}', Error: '{}'",
                __func__, chunk_path, strerror(errno));
        throw ChunkStorageException(errno, err_str);
    }

    size_t wrote_total{};
    ssize_t wrote{};
    do {
        wrote = pwrite(fh.native(), buf + wrote_total, size - wrote_total, offset + wrote_total);

        if(wrote < 0) {
            if(errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
                continue;
            auto err_str = fmt::format(
                    "{}() Failed to write chunk file. File: '{}', size: '{}', offset: '{}', Error: '{}'",
                    __func__, chunk_path, size, offset, strerror(errno));
            throw ChunkStorageException(errno, err_str);
        }
        wrote_total += wrote;
    } while(wrote_total != size);

    // 文件会关闭的，因为设置了析构函数，所以直接return
    return wrote_total;
}

ssize_t ChunkStorage::read_chunk(const string &file_path, gaofs::rpc::chnk_id_t chunk_id, char *buf, size_t size,
                                 off64_t offset) const {
    assert(offset + size <= chunksize_);
    auto chunk_path = absolute(get_chunk_path(file_path, chunk_id));

    // 生成文件对应的file handle，并且用open函数打开
    FileHandle fh(open(chunk_path.c_str(), O_RDONLY), chunk_path);
    if(!fh.valid()) {
        auto err_str = fmt::format(
                "{}() Failed to open chunk file for read. File: '{}', Error: '{}'",
                __func__, chunk_path, errno);
        throw ChunkStorageException(errno, err_str);
    }

    size_t read = 0;
    ssize_t read_total = 0;
    do {
        read = pread64(fh.native(), buf+read_total, size-read_total, offset+read_total);
        if(read == 0) {
            // read == 0 并不代表出错，可能是表示文件结束，所以不能认为出错了，直接break就好
            break;
        }

        if(read < 0) {
            if(errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
                continue;
            auto err_str = fmt::format(
                    "{}() Failed to read chunk file. File: '{}', size: '{}', offset: '{}', Error: '{}'",
                    __func__, chunk_path, size, offset, strerror(errno));
            throw ChunkStorageException(errno, err_str);
        }

#ifndef NDEBUG
        // TODO: ndebug
        if(read + read_total < size) {
            log_->debug("Read less bytes than requested: '{}'/{}. Total read was '{}'. This is not an error!",
                        read, size - read_total, size);
        }
#endif
        assert(read > 0);
        read_total += read;
    } while(read_total != size);

    return read_total;
}

// 删除所有大于chunk_id_start的块
// 如果再删除块时遇到错误，仍然会删除所有文件，然后抛出ChunkStorageException
void ChunkStorage::trim_chunk_space(const string &file_path, gaofs::rpc::chnk_id_t chunk_start) {
    auto chunk_dir = absolute(get_chunks_dir(file_path));
    // 代表目录的结束
    const fs::directory_iterator end;
    auto err_flag = false;
    for(fs::directory_iterator chunk_file(chunk_dir); chunk_file != end; chunk_file++) {
        auto chunk_path = chunk_file->path();
        auto chunk_id = std::stoul(chunk_path.filename().c_str());
        if(chunk_id >= chunk_start) {
            auto err = unlink(chunk_path.c_str());
            // ENOENT = Error No Entry
            if(err == -1 && errno != ENOENT) {
                err_flag = true;
                log_->warn(""
                           "{}() Failed to remove chunk file. File: '{}', Error: '{}'",
                           __func__, chunk_path.native(), strerror(errno));
            }
        }
    }
    if(err_flag) {
        throw ChunkStorageException(EIO,fmt::format("{}() One or more errors occurred when truncating '{}'",__func__, file_path));
    }
}

// 截断某个chunk里的数据，针对单个chunk
void ChunkStorage::truncate_chunk_file(const string &file_path, gaofs::rpc::chnk_id_t chunk_id, off_t length) {
   auto chunk_path = absolute(get_chunk_path(file_path, chunk_id));
   assert(length > 0 && static_cast<size_t>(length) <= chunksize_);
   auto err = truncate(chunk_path.c_str(), length);
   if(err == -1) {
       auto err_str = fmt::format(
               "{}() Failed to truncate chunk file. File: '{}', length: '{}', Error: '{}'",
               __func__, chunk_path, length, strerror(errno));
       throw ChunkStorageException(errno, err_str);
   }
}

ChunkStat ChunkStorage::chunk_stat() const {
    struct statfs sfs{};
    if(statfs(root_path_.c_str(), &sfs) != 0) {
        auto err_str = fmt::format(
                "{}() Failed to get filesystem statistic for chunk directory. Error: '{}'",
                __func__, strerror(errno));
        throw ChunkStorageException(errno, err_str);
    }

    // 统计各项信息
    auto bytes_total = static_cast<unsigned long long>(sfs.f_bsize) * static_cast<unsigned long long>(sfs.f_blocks);
    auto bytes_free = static_cast<unsigned long long>(sfs.f_bsize) * static_cast<unsigned long long>(sfs.f_bavail);

    return {chunksize_, bytes_total / chunksize_, bytes_free / chunksize_};
}

} // namespace gaofs::data