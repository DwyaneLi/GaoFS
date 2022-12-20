cmake_minimum_required(VERSION 3.16)
project(GaoFS)

#c++ 17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include")

# define include directories that are relevant for all targets
# 所有include文件夹
include_directories(
        ${INCLUDE_DIR}
)

# Common components
add_subdirectory(src/common)

add_subdirectory(src/client)