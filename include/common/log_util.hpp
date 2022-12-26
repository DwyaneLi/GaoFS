//
// Created by DELL on 2022/12/23.
//

#ifndef GAOFS_LOG_UTIL_HPP
#define GAOFS_LOG_UTIL_HPP

#include <spdlog/spdlog.h>

namespace gaofs::log {

spdlog::level::level_enum get_level(std::string level_str);

spdlog::level::level_enum get_level(unsigned long level);

void setup(const std::vector<std::string>& loggers, spdlog::level::level_enum level, const std::string& path);
} // namespace gaofs::log
#endif //GAOFS_LOG_UTIL_HPP
