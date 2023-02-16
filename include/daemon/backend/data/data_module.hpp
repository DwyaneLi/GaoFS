//
// Created by DELL on 2023/2/15.
//

#ifndef GAOFS_DATA_MODULE_HPP
#define GAOFS_DATA_MODULE_HPP

#include <spdlog/spdlog.h>

namespace gaofs::data {

// 该类对于data模块提供日志记录
class DataModule {

private:
    DataModule() = default;

    // 日志实例
    std::shared_ptr<spdlog::logger> log_;

public:
    static constexpr const char* LOGGER_NAME = "DataModule";

    static DataModule* getInstance() {
        static DataModule instance;
        return &instance;
    }

    // 没有左值构造
    DataModule(DataModule const&) = delete;

    void operator=(DataModule const&) = delete;

    // getter and setter
    [[nodiscard]] const std::shared_ptr<spdlog::logger>& log() const;

    void log(const std::shared_ptr<spdlog::logger>& log);
};

// 这个宏用来创建实例
#define GAOFS_DATA_MOD                                                          \
    (static_cast<gaofs::data::DataModule*>(                                     \
            gaofs::data::DataModule::getInstance()))

} // namespace gaofs::data

#endif //GAOFS_DATA_MODULE_HPP
