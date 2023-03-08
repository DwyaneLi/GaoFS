//
// Created by DELL on 2023/2/28.
//

#ifndef GAOFS_RPC_DEFS_HPP
#define GAOFS_RPC_DEFS_HPP

extern "C" {
#include <margo.h>
};

DECLARE_MARGO_RPC_HANDLER(rpc_srv_get_fs_config)

// metadata
DECLARE_MARGO_RPC_HANDLER(rpc_srv_create)

DECLARE_MARGO_RPC_HANDLER(rpc_srv_stat)

DECLARE_MARGO_RPC_HANDLER(rpc_srv_decr_size)

DECLARE_MARGO_RPC_HANDLER(rpc_srv_remove_metadata)

DECLARE_MARGO_RPC_HANDLER(rpc_srv_remove_data)

DECLARE_MARGO_RPC_HANDLER(rpc_srv_update_metadentry)

DECLARE_MARGO_RPC_HANDLER(rpc_srv_update_metadentry_size)

DECLARE_MARGO_RPC_HANDLER(rpc_srv_get_metadentry_size)

DECLARE_MARGO_RPC_HANDLER(rpc_srv_get_dirents)

DECLARE_MARGO_RPC_HANDLER(rpc_srv_get_dirents_extended)

// data
DEFINE_MARGO_RPC_HANDLER(rpc_srv_write)

DEFINE_MARGO_RPC_HANDLER(rpc_srv_read)

DEFINE_MARGO_RPC_HANDLER(rpc_srv_truncate)

DEFINE_MARGO_RPC_HANDLER(rpc_srv_get_chunk_stat)
#endif //GAOFS_RPC_DEFS_HPP
