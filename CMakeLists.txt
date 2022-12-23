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

# fmt components
add_subdirectory(external/fmt)
set_property(TARGET fmt PROPERTY POSITION_INDEPENDENT_CODE ON)

# Common components
add_subdirectory(src/common)

# Client components
add_subdirectory(src/client)

# 将从{fmt}和spdlog导入的任何CMake变量标记为高级，以便它们不会出现在CMake -gui或ccmake中。FETCHCONTENT变量也是类似的。
mark_as_advanced(REGEX "^(FETCHCONTENT|fmt|FMT|spdlog|SPDLOG)_.*$")