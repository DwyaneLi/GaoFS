//
// Created by DELL on 2022/12/18.
//

#ifndef GAOFS_PATH_UTIL_HPP
#define GAOFS_PATH_UTIL_HPP

#include <string>
#include <vector>

namespace gaofs::path {
    constexpr unsigned int max_length = 4096; // 最长长度
    constexpr char separator = '/'; // 定义路径分隔符

    bool is_relative(const std::string& path); // 是否为相对路径

    bool is_absolute(const std::string& path); // 是否为绝对路径

    bool has_trailing_slash(const std::string& path); //是否有末尾斜杠

    std::string prepend_path(const std::string& prefix_path, const char* raw_path); // 拼凑路径

    std::string  absolute_to_relative(const std::string& root_path, const std::string& absolute_path); // 绝对路径转相对路径

    std::string dirname(const std::string& path); // 返回目录名的完整路径

    std::vector<std::string> split_path(const std::string& path); //分隔路径

} // namespace gaofs::path
#endif //GAOFS_PATH_UTIL_HPP
