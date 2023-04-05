//
// Created by DELL on 2023/3/18.
//


#include <client/open_file_map.hpp>
#include <client/open_dir.hpp>
#include <client/logging.hpp>
#include <client/preload_util.hpp>

extern "C" {
#include <fcntl.h>
}

using namespace std;
namespace gaofs::filemap {

OpenFile::OpenFile(const string &path, int flags, FileType type) : path_(path), type_(type) {

    // 设置openfile对应的flag
    if(flags & O_CREAT)
        flags_[gaofs::utils::to_underlying(OpenFile_flags::creat)] = true;
    if(flags & O_APPEND)
        flags_[gaofs::utils::to_underlying(OpenFile_flags::append)] = true;
    if(flags & O_TRUNC)
        flags_[gaofs::utils::to_underlying(OpenFile_flags::trunc)] = true;
    if(flags & O_RDONLY)
        flags_[gaofs::utils::to_underlying(OpenFile_flags::rdonly)] = true;
    if(flags & O_WRONLY)
        flags_[gaofs::utils::to_underlying(OpenFile_flags::wronly)] = true;
    if(flags & O_RDWR)
        flags_[gaofs::utils::to_underlying(OpenFile_flags::rdwr)] = true;

    pos_ = 0;
}

// getter and setter
string OpenFile::path() const {
    return path_;
}

void OpenFile::path(const string& path) {
    OpenFile::path_ = path;
}

unsigned long
OpenFile::pos() {
    lock_guard<mutex> lock(pos_mutex_);
    return pos_;
}

void
OpenFile::pos(unsigned long pos) {
    lock_guard<mutex> lock(pos_mutex_);
    OpenFile::pos_ = pos;
}

// TODO: 为什么是对pos_mutex_操作
bool
OpenFile::get_flag(OpenFile_flags flag) {
    lock_guard<mutex> lock(pos_mutex_);
    return flags_[gaofs::utils::to_underlying(flag)];
}

void
OpenFile::set_flag(OpenFile_flags flag, bool value) {
    lock_guard<mutex> lock(pos_mutex_);
    flags_[gaofs::utils::to_underlying(flag)] = value;
}

FileType
OpenFile::type() const {
    return type_;
}


// OpenFileMap
OpenFileMap::OpenFileMap() : fd_idx(10000), fd_validation_needed(false) {}

shared_ptr<OpenFile> OpenFileMap::get(int fd) {
    lock_guard<recursive_mutex> lock(files_mutex_);
    auto f = files_.find(fd);
    if(f == files_.end()) {
        return nullptr;
    } else {
        return f->second;
    }
}

std::shared_ptr<OpenDir> OpenFileMap::get_dir(int dirfd) {
    auto f = get(dirfd);
    if(f == nullptr || f->type() != FileType::directory) {
        return nullptr;
    } else {
        return static_pointer_cast<OpenDir>(f);
    }
}

bool OpenFileMap::exist(int fd) {
    lock_guard<recursive_mutex> lock(files_mutex_);
    auto f = files_.find(fd);
    return !(f == files_.end());
}

int OpenFileMap::generate_fd_idx() {
    lock_guard<mutex> lock(fd_idx_mutex);
    if(fd_idx == numeric_limits<int>::max()) {
        LOG(WARNING, "File descriptor index exceeded ints max value. Setting it back to 100000");
        fd_idx = 100000;
        fd_validation_needed = true;
    }
    return fd_idx++;
}


int OpenFileMap::safe_generate_fd_idx_() {
    // 因为经过一轮后，可能会有冲突的情况，所以经过一轮后，fd_validation_needed会被设置成true
    // 然后就会循环找一个安全的
    auto fd = generate_fd_idx();
    if(fd_validation_needed) {
        while(exist(fd)) {
            fd = generate_fd_idx();
        }
    }
    return fd;
}

int OpenFileMap::add(std::shared_ptr<OpenFile> open_file) {
    auto fd = safe_generate_fd_idx_();
    lock_guard<recursive_mutex> lock(files_mutex_);
    files_.insert(make_pair(fd, open_file));
    return fd;
}

bool OpenFileMap::remove(int fd) {
    lock_guard<recursive_mutex> lock(files_mutex_);
    auto file = files_.find(fd);
    if(file == files_.end()) {
        return false;
    }

    files_.erase(fd);
    if(fd_validation_needed && files_.empty()) {
        fd_validation_needed = false;
        LOG(DEBUG, "fd_validation flag reset");
    }
    return true;
}

int OpenFileMap::dup(const int oldfd) {
    lock_guard<recursive_mutex> lock(files_mutex_);
    auto open_file = get(oldfd);
    if(open_file == nullptr) {
        errno = EBADF;
        return -1;
    }
    auto newfd = safe_generate_fd_idx_();
    files_.insert(make_pair(newfd, open_file));
    return newfd;
}

int OpenFileMap::get_fd_idx() {
    lock_guard<mutex> lock(fd_idx_mutex);
    return fd_idx;
}

int OpenFileMap::dup2(const int oldfd, const int newfd) {
    lock_guard<recursive_mutex> lock(files_mutex_);
    auto open_file = get(oldfd);
    if(open_file == nullptr) {
        errno = EBADF;
        return -1;
    }

    if(oldfd == newfd) {
        return newfd;
    }

    if(exist(newfd)) {
        remove(newfd);
    }

    if(get_fd_idx() < newfd && newfd != 0 && newfd != 1 && newfd != 2) {
        fd_validation_needed = true;
    }
    files_.insert(make_pair(newfd, open_file));
    return newfd;
}

} // namespace gaofs::filemap