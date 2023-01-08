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
        ${INCLUDE_DIR}/daemon/backend/metadata/metadata_backend.hpp
        ${INCLUDE_DIR}/daemon/backend/metadata/db.hpp
        PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/db.cpp
        )
set(DB_LINK_LIBRARIES
        PRIVATE
        # Required by RocksDB
        metadata_module
        metadata
        spdlog::spdlog
        path_util
        -ldl
        )


# 当使用RocksDB的时候，要添加对应依赖
if (GAOFS_ENABLE_ROCKSDB)
    # If it exists, RocksDB has been build with liburing support.
    # If check succeeds, we need to link with liburing with the daemon
    set(CMAKE_REQUIRED_FLAGS "-luring")
    check_cxx_source_compiles("
        #include <liburing.h>
        int main() {
          struct io_uring ring;
          io_uring_queue_init(1, &ring, 0);
          return 0;
        }
        " HAS_LIBURING)
    if (HAS_LIBURING)
        set(DB_LINK_LIBRARIES
                ${DB_LINK_LIBRARIES}
                -luring
                )
    endif ()
    # Rocksdb dependencies
    find_package(JeMalloc) # required if rocksdb has been build with jemalloc
    find_package(LZ4 REQUIRED)
    find_package(RocksDB REQUIRED)
    # Imported target
    add_library(RocksDB INTERFACE IMPORTED GLOBAL)
    target_link_libraries(RocksDB
            INTERFACE
            ${ROCKSDB_LIBRARIES}
            # rocksdb libs
            ${LZ4_LIBRARIES}
            )
    if (${JeMalloc_FOUND})
        target_link_libraries(RocksDB
                INTERFACE
                ${JEMALLOC_LIBRARIES}
                )
    endif ()
    # we cannot use target_include_directories with CMake < 3.11
    set_target_properties(RocksDB
            PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES ${ROCKSDB_INCLUDE_DIRS}
            )
    set(DB_ROCKSDB_SOURCES
            PUBLIC
            ${INCLUDE_DIR}/daemon/backend/metadata/rocksdb_backend.hpp
            PRIVATE
            ${INCLUDE_DIR}/daemon/backend/metadata/merge.hpp
            ${CMAKE_CURRENT_LIST_DIR}/rocksdb_backend.cpp
            ${CMAKE_CURRENT_LIST_DIR}/merge.cpp
            )
    target_compile_definitions(metadata_db
            PUBLIC
            GAOFS_ENABLE_ROCKSDB
            )
    set(DB_LINK_LIBRARIES
            ${DB_LINK_LIBRARIES}
            RocksDB
            )
endif ()

# 最后资源汇总
target_sources(metadata_db
        ${DB_COMMON_SOURCES}
        ${DB_ROCKSDB_SOURCES}
        )
target_link_libraries(metadata_db
        ${DB_LINK_LIBRARIES}
        )