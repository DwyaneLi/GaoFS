//
// Created by DELL on 2023/1/16.
//

#ifndef GAOFS_DAEMON_HPP
#define GAOFS_DAEMON_HPP

#include <string>
#include <spdlog/spdlog.h>

#include <config.hpp>

// TODO: 和RPC有关的头文件

#include <daemon/classes/fs_data.hpp>
#include <common/rpc/distributor.hpp>


// fs_data静态实例
#define GAOFS_DATA (static_cast<gaofs::daemon::FsData*>(gaofs::daemon::FsData::getInstance()))

#endif //GAOFS_DAEMON_HPP
