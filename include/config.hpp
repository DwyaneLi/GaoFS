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

// 创建元数据时检查是否已经存在
constexpr auto create_exist_check = true;

// 是否隐式删除data
constexpr auto implicit_data_removal = true;
} // namespace gaofs::config::metadata

namespace rpc {
constexpr auto chunksize = 524288; //chunk大小, 512kB
} // namespace gaofs::config::rpc

namespace rocksdb {
// rocksbd是否使用预写日志
constexpr  auto use_write_ahead_log = false;
} // namespace gaofs::config::rocksdb

} // namespace gaofs::config

#endif //GAOFS_CONFIG_HPP
