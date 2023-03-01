//
// Created by DELL on 2023/2/28.
//

#ifndef GAOFS_RPC_DEFS_HPP
#define GAOFS_RPC_DEFS_HPP

extern "C" {
#include <margo.h>
};

// metadata
DECLARE_MARGO_RPC_HANDLER(rpc_srv_create)

DECLARE_MARGO_RPC_HANDLER(rpc_srv_decr_size)

DECLARE_MARGO_RPC_HANDLER(rpc_srv_remove_metadata)

DECLARE_MARGO_RPC_HANDLER(rpc_srv_remove_data)


#endif //GAOFS_RPC_DEFS_HPP
