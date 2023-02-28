//
// Created by DELL on 2023/2/15.
//

#ifndef GAOFS_CHUNK_STORAGE_HPP
#define GAOFS_CHUNK_STORAGE_HPP

#include <limits>
#include <string>
#include <memory>
#include <system_error>

#include <common/common_defs.h>

/* Forward declarations */
namespace spdlog {
class logger;
}

namespace gaofs::data {

// 用来记录chunk的信息
struct ChunkStat {
    unsigned long chunk_size;
    unsigned long chunk_total;
    unsigned long chunk_free;
};

// ststem_error 的子类，用于异常提醒
class ChunkStorageException : public std::system_error {
public:
    ChunkStorageException(const int err_code, const std::string& s) : std::system_error(err_code, std::generic_category(), s) {};
};

class ChunkStorage {
private:
    std::shared_ptr<spdlog::logger> log_; // 日志

    std::string root_path_; // 根目录路径
    size_t chunksize_; // chunk大小，按理来说config里有

    // 将gaofs路径转化成本机的系统路径
    [[nodiscard]] inline std::string absolute(const std::string& internal_path) const;

    // 返回file path的目录路径，这是个绝对路径
    static inline std::string get_chunks_dir(const std::string& file_path);

    // 返回file path和其chunk id定位的对应文件路径，即chunk的路径
    static inline std::string get_chunk_path(const std::string& file_path, gaofs::rpc::chnk_id_t chunk_id);

    // 创建gaofs的块空间，在本地文件系统上创建目录
    void init_chunk_space(const std::string& file_path) const;

public:
    // 构造函数
    ChunkStorage(std::string& path, size_t chunksize);

    // 删除块目录及其所有文件，这是块目录上的递归删除操作。
    void destroy_chunk_space(const std::string& file_path) const;

    // 写入单个快文件，通常由Argobots微线程调用。
    ssize_t write_chunk(const std::string& file_path, gaofs::rpc::chnk_id_t chunk_id, const char* buf,
                        size_t size, off64_t offset) const;

    // 读取单个块文件，通常由Argobots微线程调用。
    ssize_t read_chunk(const std::string& file_path, gaofs::rpc::chnk_id_t chunk_id, char* buf,
                       size_t size, off64_t offset) const;

    // 删除所有chunk 从某个chunk id开始。
    void trim_chunk_space(const std::string& file_path, gaofs::rpc::chnk_id_t chunk_start);

    // 将单个块文件截断为给定的字节长度。
    void truncate_chunk_file(const std::string& file_path, gaofs::rpc::chnk_id_t chunk_id, off_t length);

    // 获取统计信息
    [[nodiscard]] ChunkStat chunk_stat() const;
};


} // namespace gaofs::data

#endif //GAOFS_CHUNK_STORAGE_HPP
