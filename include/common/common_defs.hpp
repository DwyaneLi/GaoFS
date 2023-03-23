//
// Created by DELL on 2023/2/15.
//

#ifndef GAOFS_COMMON_DEFS_HPP
#define GAOFS_COMMON_DEFS_HPP
namespace gaofs::rpc {

using chnk_id_t = unsigned long;

namespace tag {
constexpr auto fs_config = "rpc_srv_fs_config";
constexpr auto create = "rpc_srv_mk_node";
constexpr auto stat = "rpc_srv_stat";
constexpr auto remove_metadata = "rpc_srv_rm_metadata";
constexpr auto remove_data = "rpc_srv_rm_data";
constexpr auto decr_size = "rpc_srv_decr_size";
constexpr auto update_metadentry = "rpc_srv_update_metadentry";
constexpr auto get_metadentry_size = "rpc_srv_get_metadentry_size";
constexpr auto update_metadentry_size = "rpc_srv_update_metadentry_size";
constexpr auto get_dirents = "rpc_srv_get_dirents";
constexpr auto get_dirents_extended = "rpc_srv_get_dirents_extended";
#ifdef HAS_SYMLINKS
constexpr auto mk_symlink = "rpc_srv_mk_symlink";
#endif
constexpr auto write = "rpc_srv_write_data";
constexpr auto read = "rpc_srv_read_data";
constexpr auto truncate = "rpc_srv_trunc_data";
constexpr auto get_chunk_stat = "rpc_srv_chunk_stat";
} // namespace tag

namespace protocol {
constexpr auto ofi_psm2 = "ofi+psm2";
constexpr auto ofi_sockets = "ofi+sockets";
constexpr auto ofi_tcp = "ofi+tcp";
constexpr auto ofi_verbs = "ofi+verbs";
constexpr auto na_sm = "na+sm";
} // namespace protocol

} // namespace gaofs::rpc
#endif //GAOFS_COMMON_DEFS_HPP