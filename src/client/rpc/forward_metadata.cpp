//
// Created by DELL on 2023/3/19.
//

#include <client/rpc/forward_metadata.hpp>
#include <client/preload_util.hpp>
#include <client/logging.hpp>
#include <client/open_dir.hpp>
#include <client/rpc/rpc_types.hpp>

#include <common/rpc/rpc_util.hpp>
#include <common/rpc/distributor.hpp>
#include <common/rpc/rpc_types.hpp>

using namespace std;

namespace gaofs::rpc {

// 发送创建元数据的rpc
int forward_create(const string& path, const mode_t mode) {
    auto endp = CTX->hosts().at(CTX->distributor()->locate_file_metadata(path));

    try {
        LOG(DEBUG, "Sending RPC ...");
        auto out = ld_network_service->post<gaofs::rpc::create>(endp, path, mode).get().at(0);
        LOG(DEBUG, "Got response success: {}", out.err());

        return out.err()? out.err(): 0;
    } catch (const exception& e) {
        LOG(ERROR, "while getting rpc output");
        return EBUSY;
    }
}

// 获取metadata序列化成字符串
int forward_stat(const string& path, string& attr) {
    auto endp = CTX->hosts().at(CTX->distributor()->locate_file_metadata(path));

    try {
        LOG(DEBUG, "Sending RPC ...");

        auto out = ld_network_service->post<gaofs::rpc::stat>(endp, path).get().at(0);
        LOG(DEBUG, "Got response success: {}", out.err());

        if(out.err()) {
            return out.err();
        }
        attr = out.db_val();
    } catch (const exception& e) {
        LOG(ERROR, "while getting rpc output");
        return EBUSY;
    }

    return 0;
}

// 也一部分只负责修改metadata的size
// 因为阶段不仅要处理数据，也要处理metadata，处理数据的在forward_data里
int forward_decr_size(const string& path, size_t length) {
    auto endp = CTX->hosts().at(CTX->distributor()->locate_file_metadata(path));

        try {
            LOG(DEBUG, "Sending RPC ...");
            auto out = ld_network_service
                    ->post<gaofs::rpc::decr_size>(endp, path, length)
                    .get()
                    .at(0);

            LOG(DEBUG, "Got response success: {}", out.err());

            return out.err() ? out.err() : 0;
        } catch(const std::exception& ex) {
            LOG(ERROR, "while getting rpc output");
            return EBUSY;
        }
}

// 更新文件大小，在wirte的时候调用
int forward_update_metadentry(const string& path, const gaofs::metadata::Metadata& md,
                              const gaofs::metadata::MetadentryUpdateFlags& md_flags) {
    auto endp = CTX->hosts().at(CTX->distributor()->locate_file_metadata(path));

    try {
        LOG(DEBUG, "Sending RPC ...");
        auto out = ld_network_service
                ->post<gaofs::rpc::update_metadentry>(
                        endp, path,
                        (md_flags.link_count ? md.link_count() : 0),
                        /* mode */ 0,
                        /* uid */ 0,
                        /* gid */ 0, (md_flags.size ? md.size() : 0),
                        (md_flags.blocks ? md.blocks() : 0),
                        (md_flags.atime ? md.atime() : 0),
                        (md_flags.mtime ? md.mtime() : 0),
                        (md_flags.ctime ? md.ctime() : 0),
                        bool_to_merc_bool(md_flags.link_count),
                        /* mode_flag */ false,
                        bool_to_merc_bool(md_flags.size),
                        bool_to_merc_bool(md_flags.blocks),
                        bool_to_merc_bool(md_flags.atime),
                        bool_to_merc_bool(md_flags.mtime),
                        bool_to_merc_bool(md_flags.ctime))
                .get()
                .at(0);

        LOG(DEBUG, "Got response success: {}", out.err());

        return out.err() ? out.err() : 0;
    } catch(const std::exception& ex) {
        LOG(ERROR, "while getting rpc output");
        return EBUSY;
    }
}

// 发送RPC请求以获取当前文件大小。这是在lseek()调用期间调用的
pair<int, off64_t> forward_update_metadentry_size(const string& path, const size_t size,
                                                  const off64_t offset, const bool append_flag) {
    auto endp = CTX->hosts().at(CTX->distributor()->locate_file_metadata(path));
    try {
        LOG(DEBUG, "Sending RPC ...");

        auto out = ld_network_service
                ->post<gaofs::rpc::update_metadentry_size>(
                        endp, path, size, offset,
                        bool_to_merc_bool(append_flag))
                .get()
                .at(0);

        LOG(DEBUG, "Got response success: {}", out.err());

        if(out.err())
            return make_pair(out.err(), 0);
        else
            return make_pair(0, out.ret_size());
    } catch(const std::exception& ex) {
        LOG(ERROR, "while getting rpc output");
        return make_pair(EBUSY, 0);
    }
}

// TODO: HAS_SYMLINKS
#ifdef HAS_SYMLINKS
 // 发送一个RPC请求来创建一个符号链接。
int
forward_mk_symlink(const std::string& path, const std::string& target_path) {

    auto endp = CTX->hosts().at(CTX->distributor()->locate_file_metadata(path));

    try {
        LOG(DEBUG, "Sending RPC ...");
        // TODO(amiranda): add a post() with RPC_TIMEOUT to hermes so that we
        // can retry for RPC_TRIES (see old commits with margo)
        // TODO(amiranda): hermes will eventually provide a post(endpoint)
        // returning one result and a broadcast(endpoint_set) returning a
        // result_set. When that happens we can remove the .at(0) :/
        auto out =
                ld_network_service
                        ->post<gkfs::rpc::mk_symlink>(endp, path, target_path)
                        .get()
                        .at(0);

        LOG(DEBUG, "Got response success: {}", out.err());

        return out.err() ? out.err() : 0;
    } catch(const std::exception& ex) {
        LOG(ERROR, "while getting rpc output");
        return EBUSY;
    }
}

#endif

} // namespace gaofs::rpc
