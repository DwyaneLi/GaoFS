//
// Created by DELL on 2022/12/22.
//

#include <common/first_chunk.hpp>
#include <config.hpp>
#include <fmt/format.h>
#include <cassert>

namespace gaofs::first_chunk {

static const char MSP = '|'; // 分隔符

// 构造函数
First_chunk::First_chunk(size_t size) : size_(size) {
    assert(size_ < gaofs::config::rpc::chunksize);
}

// 通过序列构建first_chunk
First_chunk::First_chunk(const std::string &binary_str) {
    size_t read = 0;

    // 初始化size_
    auto ptr = binary_str.data();
    size_ = std::stol(ptr, &read);
    assert(read > 0);
    ptr += read;
    assert(*ptr == MSP);

    // 初始化content_
    content_ = ++ptr;
    ptr += content_.size();

    // 末尾了
    assert(*ptr == '\0');
}

// 序列化first_chunk，以便rocksdb存储
std::string First_chunk::serialize() const {
    std::string s;
    // 把size_序列化
    s += fmt::format_int(size_).c_str();
    s += MSP;
    // 把content_序列化；
    s += content_;
    return s;
}

// getter and setter
void First_chunk::size(size_t size) {
    size_ = size;
}

size_t First_chunk::size() const {
    return size_;
}

void First_chunk::content(std::string content) {
    content_ = content;
}

std::string First_chunk::content() const {
    return content_;
}

} // namespace gaofs::first_chunk


