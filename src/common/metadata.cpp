//
// Created by DELL on 2022/12/19.
//
#include <common/metadata.hpp>
#include <config.hpp>

#include <fmt/format.h>
extern "C" {
#include <sys/stat.h>
#include <unistd.h>
}

#include <ctime>
#include <cassert>

namespace gaofs::metadata {

static const char MSP = '|'; // 分隔符

// 构造函数
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

// 通过序列构建Metadata
Metadata::Metadata(const std::string &binary_str) {
    size_t read = 0;

    // 初始化mode_
    auto ptr = binary_str.data();
    mode_ = static_cast<unsigned int>(std::stoul(ptr, &read));
    assert(read > 0);
    ptr += read;
    // 用分隔符分隔
    assert(*ptr == MSP);

    // 下面类似
    size_ = std::stol(++ptr, &read);
    assert(read > 0);
    ptr += read;

    if constexpr(gaofs::config::metadata::use_atime) {
        assert(*ptr == MSP);
        atime_ = static_cast<time_t>(std::stol(++ptr, &read));
        assert(read > 0);
        ptr += read;
    }
    if constexpr(gaofs::config::metadata::use_mtime) {
        assert(*ptr == MSP);
        mtime_ = static_cast<time_t>(std::stol(++ptr, &read));
        assert(read > 0);
        ptr += read;
    }
    if constexpr(gaofs::config::metadata::use_ctime) {
        assert(*ptr == MSP);
        ctime_ = static_cast<time_t>(std::stol(++ptr, &read));
        assert(read > 0);
        ptr += read;
    }
    if constexpr(gaofs::config::metadata::use_link_cnt) {
        assert(*ptr == MSP);
        link_count_ = static_cast<nlink_t>(std::stoul(++ptr, &read));
        assert(read > 0);
        ptr += read;
    }
    if constexpr(gaofs::config::metadata::use_blocks) {
        assert(*ptr == MSP);
        blocks_ = static_cast<blkcnt_t>(std::stoul(++ptr, &read));
        assert(read > 0);
        ptr += read;
    }

#ifdef HAS_SYMLINKS
    // 读target_path
    assert(*ptr == MSP);
    target_path_ = ++ptr;

    assert(target_path_.empty() || S_ISLNK(mode_));
    ptr += target_path_.size();
#endif

    // 末尾了
    assert(*ptr == '\0');
}

// metadata 序列化
std::string Metadata::serialize() const {
    std::string s;
    // 把mode_序列化
    s += fmt::format_int(mode_).c_str();
    // 插入分隔符
    s += MSP;
    // 下面类似
    s += fmt::format_int(size_).c_str();
    if constexpr(gaofs::config::metadata::use_atime) {
        s += MSP;
        s += fmt::format_int(atime_).c_str();
    }
    if constexpr(gaofs::config::metadata::use_mtime) {
        s += MSP;
        s += fmt::format_int(mtime_).c_str();
    }
    if constexpr(gaofs::config::metadata::use_ctime) {
        s += MSP;
        s += fmt::format_int(ctime_).c_str();
    }
    if constexpr(gaofs::config::metadata::use_link_cnt) {
        s += MSP;
        s += fmt::format_int(link_count_).c_str();
    }
    if constexpr(gaofs::config::metadata::use_blocks) {
        s += MSP;
        s += fmt::format_int(blocks_).c_str();
    }

#ifdef HAS_SYMLINKS
    s += MSP;
    s += target_path_;
#endif

    return s;
}

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
} // namespace gaofs::metadata
