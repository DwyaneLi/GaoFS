//
// Created by DELL on 2023/2/27.
//

#ifndef GAOFS_UTIL_HPP
#define GAOFS_UTIL_HPP

// 主要是对host_file的读写操作

namespace gaofs::utils {

// 将守护进程的rpc地址注册到host_file
void populate_hosts_file();


// 删除整个host_file
void destroy_hosts_file();

} // namespace gaofs::utils

#endif //GAOFS_UTIL_HPP
