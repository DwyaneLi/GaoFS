//
// Created by DELL on 2023/3/18.
//

// TODO: INCLUDE
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





}