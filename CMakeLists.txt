cmake_minimum_required(VERSION 3.16)
project(GaoFS)

#c++ 17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# 设置CMAKE_MODULE_PATH，帮助找到.cmake文件
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake" ${CMAKE_MODULE_PATH})


find_package(Mercury REQUIRED)
find_package(Abt REQUIRED)
find_package(Margo REQUIRED)
find_package(Threads REQUIRED)
#find package
# 有些编译器需要额外的std::filesystem标志，例如-lstdc++fs，它会生成一个导入std::filesystem的目标，透明地处理所有这些细节
find_package(Filesystem REQUIRED)
#



#选项，使用哪个数据库，默认使用rocks db
# option(GKFS_ENABLE_PARALLAX "Enable Parallax db backend" OFF)
option(GAOFS_ENABLE_ROCKSDB "Enable ROCKSDB backend" ON)


# fmt components
add_subdirectory(external/fmt)
set_property(TARGET fmt PROPERTY POSITION_INDEPENDENT_CODE ON)

# spdlog components
add_subdirectory(external/spdlog)
set_property(TARGET spdlog PROPERTY POSITION_INDEPENDENT_CODE ON)

# 选项，是否编译hermes rpc 库，默认开
option(HERMES_MARGO_COMPATIBLE_RPCS "" ON)
add_subdirectory(external/hermes)
# 添加编译器标志
target_compile_definitions(hermes INTERFACE)

# 设置include dir
set(INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include")
# define include directories that are relevant for all targets
# 所有include文件夹
include_directories(
        ${INCLUDE_DIR}
)
# Common components
add_subdirectory(src/common)

# Daemon components
add_subdirectory(src/daemon)

# Client components
# add_subdirectory(src/client)

# 将从{fmt}和spdlog导入的任何CMake变量标记为高级，以便它们不会出现在CMake -gui或ccmake中。FETCHCONTENT变量也是类似的。
mark_as_advanced(REGEX "^(FETCHCONTENT|fmt|FMT|spdlog|SPDLOG)_.*$")