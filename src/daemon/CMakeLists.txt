
add_subdirectory(backend)

# 设置相应变量
set(DAEMON_SOURCES
        PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/handler/srv_metadata.cpp
        ${CMAKE_CURRENT_LIST_DIR}/handler/srv_management.cpp
        ${CMAKE_CURRENT_LIST_DIR}/handler/srv_data.cpp
        ${CMAKE_CURRENT_LIST_DIR}/../common/rpc/rpc_util.cpp
        ${CMAKE_CURRENT_LIST_DIR}/util.cpp
        ${CMAKE_CURRENT_LIST_DIR}/classes/fs_data.cpp
        ${CMAKE_CURRENT_LIST_DIR}/classes/rpc_data.cpp
        ${CMAKE_CURRENT_LIST_DIR}/daemon.cpp
        ${CMAKE_CURRENT_LIST_DIR}/ops/metadentry.cpp
        ${CMAKE_CURRENT_LIST_DIR}/ops/data.cpp
        PUBLIC
        ${INCLUDE_DIR}/daemon/handler/rpc_defs.hpp
        ${INCLUDE_DIR}/daemon/handler/rpc_util.hpp
        ${INCLUDE_DIR}/common/rpc/rpc_types.hpp
        ${INCLUDE_DIR}/common/rpc/rpc_util.hpp
        ${INCLUDE_DIR}/daemon/util.hpp
        ${INCLUDE_DIR}/daemon/classes/fs_data.hpp
        ${INCLUDE_DIR}/daemon/classes/rpc_data.hpp
        ${INCLUDE_DIR}/daemon/daemon.hpp
        ${INCLUDE_DIR}/daemon/ops/metadentry.hpp
        ${INCLUDE_DIR}/daemon/ops/data.hpp
        ${INCLUDE_DIR}/config.hpp
        )

set(DAEMON_LINK_LIBRARIES
        PRIVATE
        # internal libs
        metadata
        first_chunk
        metadata_db
        distributor
        log_util
        env_util
        spdlog
        # CLI11
        fmt::fmt
        path_util
        # margo libs
        ${ABT_LIBRARIES}
        mercury
        ${MARGO_LIBRARIES}
        # others
        Threads::Threads
        )


#设置需要包含的外部头文件
set(DAEMON_INCLUDE_DIRS
        PRIVATE
        )

#生成gkfs_daemon可执行文件
add_executable(gaofs_daemon)
target_sources(gaofs_daemon ${DAEMON_SOURCES})
target_link_libraries(gaofs_daemon ${DAEMON_LINK_LIBRARIES})
target_include_directories(gaofs_daemon ${DAEMON_INCLUDE_DIRS})

install(TARGETS gaofs_daemon
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        )