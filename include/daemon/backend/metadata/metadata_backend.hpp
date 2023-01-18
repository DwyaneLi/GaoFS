//
// Created by DELL on 2022/12/26.
//

#ifndef GAOFS_METADATA_BACKEND_HPP
#define GAOFS_METADATA_BACKEND_HPP

#include <memory>
#include <spdlog/spdlog.h>
#include <daemon/backend/exceptions.hpp>
#include <tuple>
#include <vector>

namespace gaofs::metadata {

// 抽象类，作为所有metadata backend的父类
class AbstractMetadataBackend {

public:
    virtual  ~AbstractMetadataBackend() = default;

    // metadata部分
    virtual std::string get(const std::string& key) const = 0;

    virtual void put(const std::string& key, const std::string& val) = 0;

    virtual void put_no_exist(const std::string& key, const std::string& val) = 0;

    virtual void remove(const std::string& key) = 0;

    virtual bool exists(const std::string& key) = 0;

    virtual void update(const std::string& old_key, const std::string& new_key, const std::string& val) = 0;

    virtual void increase_size(const std::string& key, size_t size, bool append) = 0;

    virtual void decrease_size(const std::string& key, size_t size) = 0;

    virtual std::vector<std::pair<std::string, bool>> get_dirents(const std::string& dir) const = 0;

    virtual std::vector<std::tuple<std::string, bool, size_t, time_t>> get_dirents_extended(const std::string& dir) const = 0;


    // firstchunk部分
    virtual std::string get_first_chunk(const std::string& key) const = 0;

    virtual void put_first_chunk(const std::string& key, const std::string& val) = 0;

    virtual void put_no_exist_first_chunk(const std::string& key, const std::string& val) = 0;

    virtual void remove_first_chunk(const std::string& key) = 0;

    virtual bool exists_first_chunk(const std::string& key) = 0;

    virtual void update_first_chunk(const std::string& old_key, const std::string& new_key, const std::string& val) = 0;

    // 调试部分
    virtual void iterate_all() const = 0;
};


// 模板类MetadataBackend, 对应具体数据库的类的父类
template <typename T>
class MetadataBackend : public AbstractMetadataBackend {

private:
    // 数据库路径
    std::string path;

    // 数据库日志
    std::shared_ptr<spdlog::logger> log_;

public:
    // metadata部分
    std::string get(const std::string& key) const {
        return static_cast<T const&>(*this).get_impl(key);
    }

    void put(const std::string& key, const std::string& val) {
        static_cast<T&>(*this).put_impl(key, val);
    }

    void put_no_exist(const std::string& key, const std::string& val) {
        static_cast<T&>(*this).put_no_exist_impl(key, val);
    }

    void remove(const std::string& key) {
        static_cast<T&>(*this).remove_impl(key);
    }

    bool exists(const std::string& key) {
        return static_cast<T&>(*this).exists_impl(key);
    }

    void update(const std::string& old_key, const std::string& new_key, const std::string& val) {
        static_cast<T&>(*this).update_impl(old_key, new_key, val);
    }

    void increase_size(const std::string& key, size_t size, bool append) {
        static_cast<T&>(*this).increase_size_impl(key, size, append);
    }

    void decrease_size(const std::string& key, size_t size) {
        static_cast<T&>(*this).decrease_size_impl(key, size);
    }

    std::vector<std::pair<std::string, bool>> get_dirents(const std::string& dir) const {
        return static_cast<T const&>(*this).get_dirents_impl(dir);
    }

    std::vector<std::tuple<std::string, bool, size_t, time_t>> get_dirents_extended(const std::string& dir) const {
        return static_cast<T const&>(*this).get_dirents_extended_impl(dir);
    }

    // firstchunk部分
    std::string get_first_chunk(const std::string& key) const {
        return static_cast<T const&>(*this).get_first_chunk_impl(key);
    }

    void put_first_chunk(const std::string& key, const std::string& val) {
        return static_cast<T&>(*this).put_first_chunk_impl(key, val);
    }

    void put_no_exist_first_chunk(const std::string& key, const std::string& val) {
        return static_cast<T&>(*this).put_no_exist_first_chunk_impl(key, val);
    }

    void remove_first_chunk(const std::string& key) {
        return static_cast<T&>(*this).remove_first_chunk_impl(key);
    }

    bool exists_first_chunk(const std::string& key) {
        return static_cast<T&>(*this).exists_first_chunk_impl(key);
    }

    void update_first_chunk(const std::string& old_key, const std::string& new_key, const std::string& val) {
        return static_cast<T&>(*this).update_first_chunk_impl(old_key, new_key, val);
    }

    void iterate_all() const {
        static_cast<T const&>(*this).iterate_all_impl();
    };
};

} // namespace gaofs::metadata

#endif //GAOFS_METADATA_BACKEND_HPP
