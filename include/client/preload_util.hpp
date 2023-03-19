//
// Created by DELL on 2023/3/19.
//

#ifndef GAOFS_PRELOAD_UTIL_HPP
#define GAOFS_PRELOAD_UTIL_HPP

// TODO: include
#include <common/metadata.hpp>
#include <string>
#include <iostream>
#include <map>
#include <type_traits>
#include <optional>

namespace gaofs::metadata {

struct MetadentryUpdateFlags {
    bool atime = false;
    bool mtime = false;
    bool ctime = false;
    bool uid = false;
    bool gid = false;
    bool mode = false;
    bool link_count = false;
    bool size = false;
    bool blocks = false;
    bool path = false;
};

} // gaofs::metadata

// Hermes instance
namespace hermes {
// 异步引擎
class async_engine;
}

extern std::unique_ptr<hermes::async_engine> ld_network_service;

namespace gaofs::utils {

// 获取底层结构
template <typename E>
constexpr typename std::underlying_type<E>::type to_underlying(E e) {
    return static_cast<typename std::underlying_type<E>::type>(e);
}

// 通过rpc获取metadata
std::optional<gkfs::metadata::Metadata> get_metadata(const std::string& path, bool follow_links = false);

// 把metadata转换成一个stat结构
int metadata_to_stat(const std::string& path, const gaofs::metadata::Metadata& md, struct stat& attr);

void load_hosts();

void load_forwarding_map();

std::vector<std::pair<std::string, std::string>> read_hosts_file();

void connect_to_hosts(const std::vector<std::pair<std::string, std::string>>& hosts);

} // namespace gaofs::utils

#endif //GAOFS_PRELOAD_UTIL_HPP
