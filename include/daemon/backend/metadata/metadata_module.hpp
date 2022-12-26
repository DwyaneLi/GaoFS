//
// Created by DELL on 2022/12/24.
//

#ifndef GAOFS_METADATA_MODULE_HPP
#define GAOFS_METADATA_MODULE_HPP

#include <spdlog/spdlog.h>

namespace gaofs::data {

class MetadataModule {

private:
    // 构造函数
    MetadataModule() = default;

    // 日志
    std::shared_ptr<spdlog::logger> log_;

public:
    static constexpr const char* LOGGER_NAME = "MetadataModule";

    // 获取一个全局变量实例
    static MetadataModule* getInstance() {
        static MetadataModule instance;
        return &instance;
    }

    // 无法通过MetadataModule实例来初始化
    MetadataModule(MetadataModule const&) = delete;

    // 无法通过=号赋值
    void operator=(MetadataModule const&) = delete;

    // Getter and Setter
    const std::shared_ptr<spdlog::logger>& log() const;

    void log(const std::shared_ptr<spdlog::logger>& log);
};

#define GAOFS_METADATA_MOD                                                      \
    (static_cast<gaofs::data::MetadataModule*>(                                 \
            gaofs::data::MetadataModule::getInstance()))

} // namespace gaofs::data


#endif //GAOFS_METADATA_MODULE_HPP
