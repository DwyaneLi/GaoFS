//
// Created by DELL on 2023/2/27.
//

#ifndef GAOFS_RPC_UTIL_HPP
#define GAOFS_RPC_UTIL_HPP

#include <string>

extern "C" {
#include <mercury_types.h>
#include <mercury_proc_string.h>
};

namespace gaofs::rpc {

// 将布尔变量转变成mercury中的hg_bool_t
// hg_bool_t就是一个8位的uint
hg_bool_t bool_to_merc_bool(bool state);

// 返回本地的host_name
std::string get_my_hostname(bool short_hostname = false);

// TODO: 有一个不使用的函数get_host_by_name
}
#endif //GAOFS_RPC_UTIL_HPP
