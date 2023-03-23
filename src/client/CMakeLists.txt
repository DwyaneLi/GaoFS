set(PRELOAD_SOURCES
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/rpc/forward_data.cpp
        ${CMAKE_CURRENT_LIST_DIR}/rpc/forward_metadata.cpp
        ${CMAKE_CURRENT_LIST_DIR}/rpc/forward_management.cpp
        ${CMAKE_CURRENT_LIST_DIR}/rpc/rpc_types.cpp
        ${CMAKE_CURRENT_LIST_DIR}/path.cpp
        ${CMAKE_CURRENT_LIST_DIR}/logging.cpp
        ${CMAKE_CURRENT_LIST_DIR}/syscalls/detail/syscall_info.c
        ${CMAKE_CURRENT_LIST_DIR}/open_file_map.cpp
        ${CMAKE_CURRENT_LIST_DIR}/open_dir.cpp
        ${CMAKE_CURRENT_LIST_DIR}/preload_util.cpp
        ${CMAKE_CURRENT_LIST_DIR}/preload.cpp
        ${CMAKE_CURRENT_LIST_DIR}/preload_context.cpp
    PUBLIC
        ${INCLUDE_DIR}/client/preload_util.hpp
        ${INCLUDE_DIR}/client/preload.hpp
        ${INCLUDE_DIR}/client/preload_context.hpp
        ${INCLUDE_DIR}/client/rpc/forward_data.hpp
        ${INCLUDE_DIR}/client/rpc/forward_metadata.hpp
        ${INCLUDE_DIR}/client/rpc/forward_management.hpp
        ${INCLUDE_DIR}/client/rpc/rpc_types.hpp
        ${INCLUDE_DIR}/client/preload_util.hpp
        ${INCLUDE_DIR}/client/syscalls.hpp
        ${INCLUDE_DIR}/client/open_file_map.hpp
        ${INCLUDE_DIR}/client/open_dir.hpp
        ${INCLUDE_DIR}/client/make_array.hpp
        ${INCLUDE_DIR}/client/logging.hpp
        ${INCLUDE_DIR}/client/path.hpp
        ${INCLUDE_DIR}/client/env.hpp
        ${INCLUDE_DIR}/client/syscalls/args.hpp
        ${INCLUDE_DIR}/client/syscalls/errno.hpp
        ${INCLUDE_DIR}/client/syscalls/rets.hpp
        ${INCLUDE_DIR}/client/syscalls/syscall.hpp
        ${INCLUDE_DIR}/client/syscalls/decoder.hpp
        ${INCLUDE_DIR}/client/syscalls/detail/syscall_info.h
)

set(PRELOAD_LINK_LIBRARIES
    PRIVATE
        # internal
        metadata
        distributor
        env_util
        arithmetic
        path_util
        # external
        Syscall_intercept::Syscall_intercept
        mercury
        hermes
        fmt::fmt
        Threads::Threads
        Date::TZ
        )

set(PRELOAD_INCLUDE_DIRS
        PRIVATE
        ${ABT_INCLUDE_DIRS}
        ${MARGO_INCLUDE_DIRS}
        )

add_library(gaofs_intercept SHARED)
target_sources(gaofs_intercept ${PRELOAD_SOURCES})
target_link_libraries(gaofs_intercept PRIVATE ${PRELOAD_LINK_LIBRARIES})
target_include_directories(gaofs_intercept PRIVATE ${PRELOAD_INCLUDE_DIRS})
