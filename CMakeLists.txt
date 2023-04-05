cmake_minimum_required(VERSION 3.16)
project(GaoFS)

#c++ 17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


set(WARNINGS_FLAGS "-Wall -Wextra --pedantic -Wno-unused-parameter -Wno-missing-field-initializers")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG -O3")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${WARNINGS_FLAGS} -g -O0 -DGAOFS_DEBUG_BUILD -DHERMES_DEBUG_BUILD")
set(CMAKE_CXX_FLAGS_MEMCHECK "${WARNINGS_FLAGS} -g -O0 -fsanitize=address -fno-omit-frame-pointer")
set(CMAKE_CXX_FLAGS_MAINTAINER "${WARNINGS_FLAGS} -g -O0 -pg -no-pie")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -DNDEBUG -O3")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${WARNINGS_FLAGS} -g -O0 -DGAOFS_DEBUG_BUILD")
set(CMAKE_C_FLAGS_MEMCHECK "${WARNINGS_FLAGS} -g -O0 -fsanitize=address -fno-omit-frame-pointer")
set(CMAKE_C_FLAGS_MAINTAINER "${WARNINGS_FLAGS} -g -O0 -pg -no-pie")
mark_as_advanced(CMAKE_CXX_FLAGS_MAINTAINER)

# 设置CMAKE_MODULE_PATH，帮助找到.cmake文件
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake" ${CMAKE_MODULE_PATH})


find_package(Mercury REQUIRED)
find_package(Abt REQUIRED)
find_package(Margo REQUIRED)
find_package(Syscall_intercept REQUIRED)
find_package(Threads REQUIRED)
#find package
# 有些编译器需要额外的std::filesystem标志，例如-lstdc++fs，它会生成一个导入std::filesystem的目标，透明地处理所有这些细节
find_package(Filesystem REQUIRED)
#
find_package(Date REQUIRED)

option(CREATE_CHECK_PARENTS "Check parent directory existance before creating child node" ON)
if (CREATE_CHECK_PARENTS)
    add_definitions(-DCREATE_CHECK_PARENTS)
endif ()
message(STATUS "[gaofs] Create checks parents: ${CREATE_CHECK_PARENTS}")



# 看是否支持软连接
option(SYMLINK_SUPPORT "Compile with support for symlinks" ON)
if (SYMLINK_SUPPORT)
    add_definitions(-DHAS_SYMLINKS)
endif ()
message(STATUS "[gaofs] Symlink support: ${SYMLINK_SUPPORT}")

#设置内部用的文件描述符的数量
set(MAX_INTERNAL_FDS 256 CACHE STRING "Number of file descriptors reserved for internal use")
add_definitions(-DMAX_INTERNAL_FDS=${MAX_INTERNAL_FDS})
message(STATUS "[gaofs] File descriptors reserved for internal use: ${MAX_INTERNAL_FDS}")

#获取一个进程最大可以打开的文件数量
#todo: 搞懂
execute_process(COMMAND getconf OPEN_MAX
        OUTPUT_VARIABLE GETCONF_MAX_FDS
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET)
if (NOT GETCONF_MAX_FDS)
    set(GETCONF_MAX_FDS=512)
endif ()
add_definitions(-DMAX_OPEN_FDS=${GETCONF_MAX_FDS})

option(ENABLE_CLIENT_LOG "Enable logging messages" ON)
if (ENABLE_CLIENT_LOG)
    add_definitions(-DGAOFS_ENABLE_LOGGING)
endif ()
message(STATUS "[gaofs] Client logging output: ${ENABLE_CLIENT_LOG}")

#选项，是否使用agios，默认关
option(GAOFS_ENABLE_AGIOS "Enable AGIOS scheduling library" OFF)
if (GAOFS_ENABLE_AGIOS)
    find_package(AGIOS REQUIRED)
endif ()


#设置最长log大小
set(CLIENT_LOG_MESSAGE_SIZE 1024 CACHE STRING "Maximum size of a log message in the client library")
add_definitions(-DLIBGAOFS_LOG_MESSAGE_SIZE=${CLIENT_LOG_MESSAGE_SIZE})
message(STATUS "[gaofs] Maximum log message size in the client library: ${CLIENT_LOG_MESSAGE_SIZE}")
mark_as_advanced(CLIENT_LOG_MESSAGE_SIZE)

#选项，使用哪个数据库，默认使用rocks db
# option(GAOFS_ENABLE_PARALLAX "Enable Parallax db backend" OFF)
option(GAOFS_ENABLE_ROCKSDB "Enable ROCKSDB backend" ON)


# CLI11库编译
add_library(CLI11 INTERFACE)
# we cannot use target_include_directories with CMake < 3.11
set_target_properties(CLI11
        PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/external"
        )


# fmt components
add_subdirectory(external/fmt)
set_property(TARGET fmt PROPERTY POSITION_INDEPENDENT_CODE ON)


# spdlog components
add_subdirectory(external/spdlog)
set_property(TARGET spdlog PROPERTY POSITION_INDEPENDENT_CODE ON)


set(HERMES_LOGGING ON CACHE BOOL "" FORCE)
option(HERMES_LOGGING_FMT_USE_BUNDLED "" OFF)
option(HERMES_LOGGING_FMT_HEADER_ONLY "" OFF)


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
add_subdirectory(src/client)

# 将从{fmt}和spdlog导入的任何CMake变量标记为高级，以便它们不会出现在CMake -gui或ccmake中。FETCHCONTENT变量也是类似的。
mark_as_advanced(REGEX "^(FETCHCONTENT|fmt|FMT|spdlog|SPDLOG)_.*$")