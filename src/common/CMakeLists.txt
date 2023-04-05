include(FetchContent)

# 添加arithmetic模块
add_subdirectory(arithmetic)

# 编译distributor库
add_library(distributor STATIC)
set_property(TARGET distributor PROPERTY POSITION_INDEPENDENT_CODE ON)
target_sources(distributor
        PUBLIC
        ${INCLUDE_DIR}/common/rpc/distributor.hpp
        PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/rpc/distributor.cpp
        )

#编译statistic静态库
add_library(statistics STATIC)
set_property(TARGET statistics PROPERTY POSITION_INDEPENDENT_CODE ON)
target_sources(statistics
        PUBLIC
        ${INCLUDE_DIR}/common/statistics/stats.hpp
        PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/statistics/stats.cpp
        )

if(GKFS_ENABLE_PROMETHEUS)
    find_package(CURL REQUIRED)
    find_package(prometheus-cpp REQUIRED)
    set(PROMETHEUS_LINK_LIBRARIES
            prometheus-cpp::pull
            prometheus-cpp::push
            prometheus-cpp::core
            curl)
    #如果要用pormethus就要包含对应头文件
    target_include_directories(statistics PRIVATE ${prometheus-cpp_INCLUDE_DIR})
endif()

target_link_libraries(statistics
        PRIVATE
        ${PROMETHEUS_LINK_LIBRARIES}
        )

# 编译path_util库
add_library(path_util STATIC)
set_property(TARGET path_util PROPERTY POSITION_INDEPENDENT_CODE ON)
target_sources(path_util
        PUBLIC
        ${INCLUDE_DIR}/common/path_util.hpp
        PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/path_util.cpp
        )

# 编译metadata模块
add_library(metadata STATIC)
set_property(TARGET metadata PROPERTY POSITION_INDEPENDENT_CODE ON)
target_sources(metadata
        PUBLIC
        ${INCLUDE_DIR}/common/metadata.hpp
        PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/metadata.cpp
        ${INCLUDE_DIR}/config.hpp
        )
target_link_libraries(metadata
        PRIVATE
        fmt::fmt
        )

# 编译first_chunk模块
add_library(first_chunk STATIC)
set_property(TARGET first_chunk PROPERTY POSITION_INDEPENDENT_CODE ON)
target_sources(first_chunk
        PUBLIC
        ${INCLUDE_DIR}/common/first_chunk.hpp
        PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/first_chunk.cpp
        ${INCLUDE_DIR}/config.hpp
        )
target_link_libraries(first_chunk
        PRIVATE
        fmt::fmt
        )

# 编译env_util模块
add_library(env_util STATIC)
set_property(TARGET env_util PROPERTY POSITION_INDEPENDENT_CODE ON)
target_sources(env_util
        PUBLIC
        ${INCLUDE_DIR}/common/env_util.hpp
        PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/env_util.cpp
        ${INCLUDE_DIR}/config.hpp
        )

# 利用FetchContent加载spdlog
#FetchContent_Declare(spdlog
#        SOURCE_DIR ${CMAKE_SOURCE_DIR}/external/spdlog
#        )
#FetchContent_MakeAvailable(spdlog)

# 编译log_util模块
add_library(log_util STATIC)
set_property(TARGET log_util PROPERTY POSITION_INDEPENDENT_CODE ON)
target_sources(log_util
        PUBLIC
        ${INCLUDE_DIR}/common/log_util.hpp
        PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/log_util.cpp
        )
target_link_libraries(log_util
        PRIVATE
        spdlog::spdlog
        )



