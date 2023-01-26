//
// Created by DELL on 2023/1/20.
//

#include <daemon/ops/metadentry.hpp>
#include <daemon/backend/metadata/db.hpp>
#include <daemon/backend/exceptions.hpp>

namespace gaofs::metadata {

std::string get_str(const std::string& path) {
    return GAOFS_DATA->mdb()->get(path);
}

Metadata get(const std::string& path) {
    return Metadata(get_str(path));
}

size_t get_size(const std::string& path) {
    return get(path).size();
}

std::vector<std::pair<std::string, bool>> get_dirents(const std::string& dir) {
    return GAOFS_DATA->mdb()->get_dirents(dir);
}

std::vector<std::tuple<std::string, bool, size_t, time_t>> get_dirents_extended(const std::string& dir) {
    return GAOFS_DATA->mdb()->get_dirents_extended(dir);
}

void create(const std::string& path, Metadata& md) {
    if (GAOFS_DATA->atime_state() || GAOFS_DATA->mtime_state() || GAOFS_DATA->ctime_state()) {
        time_t time;
        std::time(&time);

        if (GAOFS_DATA->mtime_state()) {
            md.mtime(time);
        }
        if (GAOFS_DATA->ctime_state()) {
            md.ctime(time);
        }
        if (GAOFS_DATA->atime_state()) {
            md.atime(time);
        }
    }
    if (gaofs::config::metadata::create_exist_check) {
        GAOFS_DATA->mdb()->put_no_exist(path, md.serialize());
    } else {
        GAOFS_DATA->mdb()->put(path, md.serialize());
    }
}

void update(const std::string& path, Metadata& md) {
    GAOFS_DATA->mdb()->update(path, path, md.serialize());
}

void update_size(const std::string& path, size_t io_size, off64_t offset, bool append) {
    GAOFS_DATA->mdb()->increase_size(path, io_size + offset, append);
}

void remove(const std::string& path) {
    //尝试从kv存储删除元数据，但捕获NotFoundException
    //在这种情况下不会报错，因为可以通过广播来捕获所有的remove
    //数据块，但只有一个节点将保存kv存储条目。
    try {
        GAOFS_DATA->mdb()->remove(path);
    } catch (const gaofs::db_exception::NotFoundException& e) {

    }
}

std::string get_str_first_chunk(const std::string& path) {
    return GAOFS_DATA->mdb()->get_first_chunk(path)
}

First_chunk get_first_chunk(const std::string& path) {
    return First_chunk(get_str_first_chunk(path));
}

size_t get_size_first_chunk(const std::string& path) {
    return get_first_chunk(path).size();
}

std::string get_content_first_chunk(const std::string& path) {
    return get_first_chunk(path).content();
}

void create_first_chunk(const std::string& path, First_chunk& firstChunk) {
    if(gaofs::config::metadata::create_exist_check) {
        GAOFS_DATA->mdb()->put_no_exist_first_chunk(path, firstChunk.serialize());
    } else {
        GAOFS_DATA->mdb()->put_first_chunk(path, firstChunk.serialize());
    }
}

void update_first_chunk(const std::string& path, First_chunk& firstChunk) {
    GAOFS_DATA->mdb()->update_first_chunk(path, path, firstChunk.serialize());
}

void remove_first_chunk(const std::string& path) {
    GAOFS_DATA->mdb()->remove_first_chunk(path);
}

} // namespace gaofs::metadata


