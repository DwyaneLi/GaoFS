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
        )