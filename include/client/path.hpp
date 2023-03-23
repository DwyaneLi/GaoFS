//
// Created by DELL on 2022/12/18.
//

#ifndef GAOFS_PATH_HPP
#define GAOFS_PATH_HPP

#include <string>
#include <vector>

namespace gaofs::path {

// 看path中前几块和components中相匹配，并且path到底有几块
unsigned int match_components(const std::string& path, unsigned int& path_components,
                              const std::vector<std::string>& components);

// 将path的规范形式表示填充到resolved中，如果在gekkofs的命名空间内则返回true，否则返回false
bool resolve(const std::string& path, std::string& resolved,
             bool resolve_last_link = true);

std::string get_sys_cwd();

void set_sys_cwd(const std::string& path);

void set_env_cwd(const std::string& path);

void unset_env_cwd();

void init_cwd();

void set_cwd(const std::string& path, bool internal);

} // namespace gaofs::path

#endif //GAOFS_PATH_HPP
