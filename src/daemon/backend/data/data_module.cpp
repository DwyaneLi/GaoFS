//
// Created by DELL on 2023/2/15.
//

#include <daemon/backend/data/data_module.hpp>

namespace gaofs::data {

const std::shared_ptr<spdlog::logger> & DataModule::log() const {
    return log_;
}

void DataModule::log(const std::shared_ptr<spdlog::logger> &log) {
    log_ = log;
}

} // namespace gaofs::data


