# 编译metadata_module库
add_library(metadata_module STATIC)
target_sources(metadata_module
        PUBLIC
        ${INCLUDE_DIR}/daemon/backend/metadata/metadata_module.hpp
        PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/metadata_module.cpp
        )
target_link_libraries(metadata_module
        PUBLIC
        spdlog::spdlog
        )

# 编译metadata_db静态库
add_library(metadata_db STATIC)
set(DB_COMMON_SOURCES
        PUBLIC
        ${INCLUDE_DIR}/daemon/backend/exceptions.hpp
        )
set(DB_LINK_LIBRARIES
        PRIVATE
        # Required by RocksDB
        metadata_module
        metadata
        spdlog
        path_util
        )
target_sources(metadata_db
        ${DB_COMMON_SOURCES}
        )
target_link_libraries(metadata_db
        ${DB_LINK_LIBRARIES}
        )