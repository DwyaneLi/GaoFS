//
// Created by DELL on 2022/12/22.
//

#ifndef GAOFS_FIRST_CHUNK_HPP
#define GAOFS_FIRST_CHUNK_HPP

#include <string>
#include <sys/types.h>
#include <config.hpp>

namespace gaofs::first_chunk {

class First_chunk {
private:
    size_t size_{}; // 存储内容的大小
    std::string content_; // 存储的内容

public:
    // 构造函数
    First_chunk() = default;

    explicit First_chunk(size_t size);

    // 利用序列初始化First_chunk;
    explicit First_chunk(const std::string& binary_str);

    // 序列化first_chunk
    std::string serialize() const;

    // getter and setter
    size_t size() const;

    void size(size_t size);

    std::string content() const;

    void content(std::string content);
};

} // namespace gaofs::first_chunk
#endif //GAOFS_FIRST_CHUNK_HPP
