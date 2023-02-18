//
// Created by DELL on 2023/2/16.
//

#ifndef GAOFS_FILE_HANDLE_HPP
#define GAOFS_FILE_HANDLE_HPP

#include <daemon/backend/data/data_module.hpp>
extern "C" {
#include <unistd.h>
};

// 文件句柄类封装文件描述符，允许RAII关闭文件描述符。
class FileHandle {

private:
    constexpr static const int init_value{-1};

    int fd_{init_value}; // 文件描述符
    std::string path_{}; // 文件路径

public:

    FileHandle() = default;

    // 构造函数
    FileHandle(int fd, std::string& path) noexcept : fd_(fd), path_(path) {}

    // 移动构造函数
    FileHandle(FileHandle&& rhs) = default;

    // 拷贝构造函数
    FileHandle(const FileHandle& other) = delete;

    FileHandle& operator=(FileHandle&& rhs) = default;

    FileHandle& operator=(const FileHandle& other) = delete;

    // 判断fd是否有效，不是-1就认为有效
    [[nodiscard]] bool valid() const noexcept {
        return fd_ != init_value;
    }

    // 重载bool类型转化 bool a = fh可以认为 bool a = fh.valid();
    explicit operator bool() const noexcept {
        return valid();
    }


    // 返回fd_
    [[nodiscard]] int native() const noexcept {
        return fd_;
    }

    // 关闭文件，并把fd_回复默认
    bool close() noexcept {
        if(fd_ != init_value) {
            if(::close(fd_) < 0) {
                GAOFS_DATA_MOD->log()->warn(
                        "{}() Failed to close file descriptor '{}' path '{}' errno '{}'",
                        __func__, fd_, path_, ::strerror(errno)
                        );
                return false;
            }
            fd_ = init_value;
            return true;
        }
    }

    // 析构函数
    ~FileHandle() {
        if(fd_ != init_value)
            close();
    }
};

#endif //GAOFS_FILE_HANDLE_HPP
