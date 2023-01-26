//
// Created by DELL on 2023/1/20.
//

#ifndef GAOFS_METADENTRY_HPP
#define GAOFS_METADENTRY_HPP

#include <daemon/daemon.hpp>
#include <common/metadata.hpp>
#include <common/first_chunk.hpp>

namespace gaofs::metadata {

// metadata部分
Metadata get(const std::string& path);

std::string get_str(const std::string& path);

size_t get_size(const std::string& path);

std::vector<std::pair<std::string, bool>> get_dirents(const std::string& dir);

std::vector<std::tuple<std::string, bool, size_t, time_t>> get_dirents_extended(const std::string& dir);

void create(const std::string& path, Metadata& md);

void update(const std::string& path, Metadata& md);

void update_size(const std::string& path, size_t io_size, off_t offset, bool append);

void remove(const std::string& path);

// firstchunk部分

First_chunk get_first_chunk(const std::string& path);

std::string get_str_first_chunk(const std::string& path);

size_t get_size_first_chunk(const std::string& path);

std::string get_content_first_chunk(const std::string& path);

void create_first_chunk(const std::string& path, First_chunk& firstChunk);

void update_first_chunk(const std::string& path, First_chunk& firstChunk);

void remove_first_chunk(const std::string& path);

} //namespace gaofs::metadata

#endif //GAOFS_METADENTRY_HPP
