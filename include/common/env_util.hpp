//
// Created by DELL on 2022/12/23.
//

#ifndef GAOFS_ENV_UTIL_HPP
#define GAOFS_ENV_UTIL_HPP

#include <string>

namespace gaofs::env {

// 获取环境变量的值
std::string get_var(const std::string& name, const std::string& default_value = "");

} // namespace gaofs::env
#endif //GAOFS_ENV_UTIL_HPP
