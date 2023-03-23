//
// Created by DELL on 2023/3/18.
//

#ifndef GAOFS_OPEN_FILE_MAP_HPP
#define GAOFS_OPEN_FILE_MAP_HPP


#include <map>
#include <mutex>
#include <memory>
#include <atomic>
#include <array>

namespace gaofs::filemap {

// 里面的某些方法要用到opendir
class OpenDir;

enum class OpenFile_flags {
    append = 0,
    creat,
    trunc,
    rdonly, // 只读
    wronly, // 只写
    rdwr,   // 可读可写
    cloexec,
    flag_count // 用来计数
};

enum class FileType {
    regular,
    directory
};


class OpenFile {

protected:
    FileType type_;
    std::string path_; // 相对于挂载路径的path
    std::array<bool, static_cast<int >(OpenFile_flags::flag_count)> flags_ = {{false}};
    unsigned long pos_;

    // 对于文件的互斥锁
    std::mutex pos_mutex_;
    std::mutex flag_mutex_;

public:
    OpenFile(const std::string& path, int flags, FileType type = FileType::regular);

    // 析构函数
    ~OpenFile() = default;

    // getter and setter
    std::string path() const;

    void path(const std::string& path_);

    unsigned long pos();

    void pos(unsigned long pos_);

    bool get_flag(OpenFile_flags flag);

    void set_flag(OpenFile_flags flag, bool value);

    FileType type() const;
};

class OpenFileMap {

private:
    std::map<int, std::shared_ptr<OpenFile>> files_; // 打开的文件
    std::recursive_mutex files_mutex_;

    int safe_generate_fd_idx_();

    int fd_idx;
    std::mutex fd_idx_mutex;
    std::atomic<bool> fd_validation_needed; // 原子类，保护这个量

public:
    OpenFileMap();

    std::shared_ptr<OpenFile> get(int fd);

    std::shared_ptr<OpenDir> get_dir(int dirfd);

    bool exist(int fd);

    int add(std::shared_ptr<OpenFile> open_file);

    bool remove(int fd);

    int dup(const int oldfd);

    int dup2(const int oldfd, const int newfd);

    int generate_fd_idx();

    int get_fd_idx();
};

} // namespace gaofs::filemap

#endif //GAOFS_OPEN_FILE_MAP_HPP
