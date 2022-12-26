//
// Created by DELL on 2022/12/24.
//

#include <daemon/backend/metadata/metadata_module.hpp>

namespace gaofs::data {

// Getter and Setter
const std::shared_ptr<spdlog::logger> &MetadataModule::log() const {
    return log_;
}

void MetadataModule::log(const std::shared_ptr<spdlog::logger> &log) {
    MetadataModule::log_ = log;
}

} // namespace gaofs::data
