//
// Created by DELL on 2023/2/28.
//

#ifndef GAOFS_RPC_TYPES_HPP
#define GAOFS_RPC_TYPES_HPP

extern "C" {
// 它提供了hg_string_t和hg_const_string_t类型，它们分别是char*和const char*的类型defs，必须用来表示以空结束的字符串。
#include <mercury_proc_string.h>

// 提供了mercury_gen_proc
#include <margo.h>
};

// 其实就是利用MERCURY_GEN_PROC设置输入输出的参数结构

MERCURY_GEN_PROC(rpc_config_out_t, ((hg_const_string_t)(mountdir))((hg_const_string_t)(rootdir))
        ((hg_bool_t)(atime_state))((hg_bool_t)(mtime_state))((hg_bool_t)(ctime_state))
        ((hg_bool_t)(link_cnt_state))((hg_bool_t)(blocks_state))((hg_uint32_t)(uid))
        ((hg_uint32_t)(gid)))


// MetaData部分

MERCURY_GEN_PROC(rpc_err_out_t, ((hg_int32_t)(err)))

// rpc_srv_create
MERCURY_GEN_PROC(rpc_mk_node_in_t, ((hg_const_string_t)(path))((uint32_t)(mode)))

// rpc_srv_stat
MERCURY_GEN_PROC(rpc_path_only_in_t, ((hg_const_string_t)(path)))
MERCURY_GEN_PROC(rpc_stat_out_t, ((hg_int32_t)(err))((hg_const_string_t)(db_val)))

// rpc_srv_decr_size
MERCURY_GEN_PROC(rpc_trunc_in_t, ((hg_const_string_t)(path))((hg_uint64_t)(length)))

// rpc_srv_remove_metadata
MERCURY_GEN_PROC(rpc_rm_node_in_t, ((hg_const_string_t)(path)))
MERCURY_GEN_PROC(rpc_rm_metadata_out_t, ((hg_int32_t)(err))((hg_int64_t)(size))((hg_uint32_t)(mode)))

// rpc_srv_update_metadentry
MERCURY_GEN_PROC(rpc_update_metadentry_in_t, ((hg_const_string_t)(path))
        ((uint64_t)(nlink))((hg_uint32_t)(mode))((hg_uint32_t)(uid))
        ((hg_uint32_t)(gid))((hg_int64_t)(size))((hg_int64_t)(blocks))
        ((hg_int64_t)(atime))((hg_int64_t)(mtime))((hg_int64_t)(ctime))
        ((hg_bool_t)(nlink_flag))((hg_bool_t)(mode_flag))((hg_bool_t)(size_flag))
        ((hg_bool_t)(block_flag))((hg_bool_t)(atime_flag))((hg_bool_t)(mtime_flag))
        ((hg_bool_t)(ctime_flag)))

// rpc_srv_update_metadentry_size
MERCURY_GEN_PROC(rpc_update_metadentry_size_in_t, ((hg_const_string_t)(path))
                ((hg_uint64_t)(size))((hg_int64_t)(offset))((hg_bool_t)(append)))
MERCURY_GEN_PROC(rpc_update_metadentry_size_out_t, ((hg_int32_t)(err))((hg_int64_t)(ret_size)))

// rpc_srv_get_metadentry_size
MERCURY_GEN_PROC(rpc_get_metadentry_size_out_t, ((hg_int32_t)(err))((hg_int64_t)(ret_size)))

// rpc_srv_get_dirents
MERCURY_GEN_PROC(rpc_get_dirents_in_t, ((hg_const_string_t)(path))((hg_bulk_t)(bulk_handle)))
MERCURY_GEN_PROC(rpc_get_dirents_out_t, ((hg_int32_t)(err))((hg_size_t)(dirents_size)))

// data
MERCURY_GEN_PROC(rpc_data_out_t, ((int32_t)(err))((hg_size_t)(io_size)))

// rpc_srv_write_data
MERCURY_GEN_PROC(rpc_write_data_in_t, ((hg_const_string_t)(path))((int64_t)(offset))
        ((hg_uint64_t)(host_id))((hg_uint64_t)(host_size))((hg_uint64_t)(chunk_n))
        ((hg_uint64_t)(chunk_start))((hg_uint64_t)(chunk_end))((hg_uint64_t)(total_chunk_size))
        ((hg_bulk_t)(bulk_handle)))

// rpc_srv_read_data
MERCURY_GEN_PROC(rpc_read_data_in_t, ((hg_const_string_t)(path))((int64_t)(offset))
        ((hg_uint64_t)(host_id))((hg_uint64_t)(host_size))((hg_uint64_t)(chunk_n))
        ((hg_uint64_t)(chunk_start))((hg_uint64_t)(chunk_end))((hg_uint64_t)(total_chunk_size))
        ((hg_bulk_t)(bulk_handle)))

// rpc_srv_get_chunk_stat
MERCURY_GEN_PROC(rpc_chunk_stat_in_t, ((hg_int32_t)(dummy)))

MERCURY_GEN_PROC(rpc_chunk_stat_out_t, ((hg_int32_t)(err))((hg_uint64_t)(chunk_size))
        ((hg_uint64_t)(chunk_total))((hg_uint64_t)(chunk_free)))

#endif //GAOFS_RPC_TYPES_HPP
