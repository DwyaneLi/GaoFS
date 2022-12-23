//
// Created by DELL on 2022/12/17.
//

#ifndef GAOFS_CONFIG_HPP
#define GAOFS_CONFIG_HPP

namespace gaofs::config {

namespace metadata {

// metadata处用的设置
constexpr auto use_atime = false;
constexpr auto use_ctime = false;
constexpr auto use_mtime = false;
constexpr auto use_link_cnt = false;
constexpr auto use_blocks = false;

} // namespace gaofs::config::metadata

namespace rpc {
    constexpr auto chunksize = 524288; //chunk大小, 512kB
}

} // namespace gaofs::config

#endif //GAOFS_CONFIG_HPP
