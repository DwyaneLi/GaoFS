//
// Created by DELL on 2022/12/19.
//
#include <common/metadata.hpp>
#include <config.h>

extern "C" {
#include <sys/stat.h>
#include <unistd.h>
}

#include <ctime>
#include <cassert>

namespace gaofs::metadata {

static const char MSP = '|'; // 分隔符

Metadata::Metadata(mode_t mode) : atime_(), mtime_(), ctime_(), mode_(mode), link_count_(0), size_(0), blocks_(0) {
    assert(S_ISDIR(mode_) || S_ISREG(mode_));
}

#ifdef HAS_SYMLINKS
Metadata::Metadata(const mode_t mode, const std::string& target_path) : atime_(), mtime_(), ctime_(), mode_(mode), link_count_(0), size_(0),
      blocks_(0), target_path_(target_path) {
    assert(S_ISLNK(mode_) || S_ISDIR(mode_) || S_ISREG(mode_));
    // target_path should be there only if this is a link
    assert(target_path_.empty() || S_ISLNK(mode_));
    // target_path should be absolute
    assert(target_path_.empty() || target_path_[0] == '/');
}
#endif

void Metadata::init_ACM_time() {
    std::time_t time;
    std::time(&time);
    atime_ = time;
    mtime_ = time;
    ctime_ = time;
}

void Metadata::update_ACM_time(bool a, bool c, bool m) {
    std::time_t time;
    std::time(&time);
    if(a)
        atime_ = time;
    if(c)
        ctime_ = time;
    if(m)
        mtime_ = time;
}

//-------------------------------------------- GETTER/SETTER

time_t Metadata::atime() const {
    return atime_;
}

void Metadata::atime(time_t atime) {
    Metadata::atime_ = atime;
}

time_t Metadata::mtime() const {
    return mtime_;
}

void Metadata::mtime(time_t mtime) {
    Metadata::mtime_ = mtime;
}

time_t Metadata::ctime() const {
    return ctime_;
}

void Metadata::ctime(time_t ctime) {
    Metadata::ctime_ = ctime;
}

mode_t Metadata::mode() const {
    return mode_;
}

void Metadata::mode(mode_t mode) {
    Metadata::mode_ = mode;
}

nlink_t Metadata::link_count() const {
    return link_count_;
}

void Metadata::link_count(nlink_t link_count) {
    Metadata::link_count_ = link_count;
}

size_t Metadata::size() const {
    return size_;
}

void Metadata::size(size_t size) {
    Metadata::size_ = size;
}

blkcnt_t Metadata::blocks() const {
    return blocks_;
}

void Metadata::blocks(blkcnt_t blocks) {
    Metadata::blocks_ = blocks;
}

#ifdef HAS_SYMLINKS

std::string Metadata::target_path() const {
    assert(!target_path_.empty());
    return target_path_;
}

void Metadata::target_path(const std::string& target_path) {
    // target_path should be there only if this is a link
    assert(target_path.empty() || S_ISLNK(mode_));
    // target_path should be absolute
    assert(target_path.empty() || target_path[0] == '/');
    target_path_ = target_path;
}

bool Metadata::is_link() const {
    return S_ISLNK(mode_);
}

#endif
} // gaofs::metadata
