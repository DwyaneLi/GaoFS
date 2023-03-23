//
// Created by DELL on 2023/3/19.
//

#include <hermes.hpp>
#include <client/rpc/rpc_types.hpp>

//==============================================================================
// register request types so that they can be used by users and the engine
//
void
hermes::detail::register_user_request_types() {
    (void) registered_requests().add<gaofs::rpc::fs_config>();
    (void) registered_requests().add<gaofs::rpc::create>();
    (void) registered_requests().add<gaofs::rpc::stat>();
    (void) registered_requests().add<gaofs::rpc::remove_metadata>();
    (void) registered_requests().add<gaofs::rpc::decr_size>();
    (void) registered_requests().add<gaofs::rpc::update_metadentry>();
    (void) registered_requests().add<gaofs::rpc::get_metadentry_size>();
    (void) registered_requests().add<gaofs::rpc::update_metadentry_size>();

#ifdef HAS_SYMLINKS
    (void) registered_requests().add<gaofs::rpc::mk_symlink>();
#endif // HAS_SYMLINKS
    (void) registered_requests().add<gaofs::rpc::remove_data>();
    (void) registered_requests().add<gaofs::rpc::write_data>();
    (void) registered_requests().add<gaofs::rpc::read_data>();
    (void) registered_requests().add<gaofs::rpc::trunc_data>();
    (void) registered_requests().add<gaofs::rpc::get_dirents>();
    (void) registered_requests().add<gaofs::rpc::chunk_stat>();
    (void) registered_requests().add<gaofs::rpc::get_dirents_extended>();
}