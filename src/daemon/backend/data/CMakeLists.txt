# 编译data_module库
add_library(data_module
        STATIC
        )

target_sources(data_module
        PUBLIC
        ${INCLUDE_DIR}/daemon/backend/data/data_module.hpp
        PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/data_module.cpp
        )

target_link_libraries(data_module
        PUBLIC
        spdlog
        )


# 编译storage库
add_library(storage
        STATIC)

target_sources(storage
        PUBLIC
        ${INCLUDE_DIR}/daemon/backend/data/chunk_storage.hpp
        PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/chunk_storage.cpp
        ${INCLUDE_DIR}/common/common_defs.hpp
        ${INCLUDE_DIR}/daemon/backend/data/file_handle.hpp
        )

target_link_libraries(storage
        PRIVATE
        spdlog
        data_module
        path_util
        stdc++fs
        -ldl
        )