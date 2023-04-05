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
#include <set>
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

// 更新metadata dentry
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

// 更新文件大小，在wirte的时候调用
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

// 发送RPC请求以获取当前文件大小。这是在lseek()调用期间调用的
pair<int, off64_t> forward_get_metadentry_size(const string& path) {
    auto endp = CTX->hosts().at(CTX->distributor()->locate_file_metadata(path));

    try {
        LOG(DEBUG, "Sending RPC ...");
        auto out = ld_network_service
                ->post<gaofs::rpc::get_metadentry_size>(endp, path)
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

// 为删除请求发送一个RPC。这将删除元数据和所有可能分布在多个守护进程中的数据块。
// 对小文件(file_size / chunk_size) < number_of_daemons进行了优化，其中不使用
// 广播到所有daemons来删除所有块。否则，将向所有守护进程广播。
int forward_remove(const string& path) {
    auto metadata_host_id = CTX->distributor()->locate_file_metadata(path);
    auto endp_metadata = CTX->hosts().at(metadata_host_id);
    int64_t size = 0;
    uint32_t mode = 0;

    // 发送rpc请求来删除metadata
    try {
        LOG(DEBUG, "Sending RPC ...");
        auto out =
                ld_network_service->post<gaofs::rpc::remove_metadata>(endp_metadata, path)
                        .get()
                        .at(0);

        LOG(DEBUG, "Got response success: {}", out.err());

        if(out.err())
            return out.err();
        // 获取文件数据的大小和mode
        size = out.size();
        mode = out.mode();
    } catch(const exception& e) {
        LOG(ERROR, "while getting rpc output");
        return EBUSY;
    }
    // 如果文件数据量为0，直接退出，不要删数据了。
    if(!(S_ISREG(mode) && (size != 0)))
        return 0;

    vector<hermes::rpc_handle<gaofs::rpc::remove_data>> handles;

    // 对于小文件
    if(static_cast<size_t>(size / gaofs::config::rpc::chunksize) < CTX->hosts().size()) {
        try {
            //LOG(DEBUG, "Sending RPC to host: {}", endp_metadata.to_string());
            gaofs::rpc::remove_data::input in(path);
            // 删除和metadata一起的数据
            //handles.emplace_back(ld_network_service->post<gaofs::rpc::remove_data>(endp_metadata, in));

            uint64_t chnk_stat = 0;
            uint64_t chnk_end = size / gaofs::config::rpc::chunksize;
            set<gaofs::rpc::host_t> target_host_ids;

            for(uint64_t chnk_id = chnk_stat; chnk_id <= chnk_end; chnk_id++) {
                const auto chnk_host_id = CTX->distributor()->locate_data(path, chnk_id);

                if constexpr(gaofs::config::metadata::implicit_data_removal) {
                    if(chnk_host_id == metadata_host_id)
                        continue;
                }

                target_host_ids.insert(chnk_host_id);
            }

            for(auto host_id = target_host_ids.cbegin(); host_id != target_host_ids.cend(); host_id++) {
                const auto endp_chnk = CTX->hosts().at(*host_id);
                LOG(DEBUG, "Sending RPC to host: {}", endp_chnk.to_string());
                handles.emplace_back(
                        ld_network_service->post<gaofs::rpc::remove_data>(
                                endp_chnk, in));
            }
        } catch (const exception& e) {
            LOG(ERROR,
                "Failed to forward non-blocking rpc request reduced remove requests");
            return EBUSY;
        }
    } else {
        // 大文件对所有host广播，因为每个结点都可能有文件块
        for(const auto endp : CTX->hosts()) {
            try {
                LOG(DEBUG, "Sending RPC to host: {}", endp.to_string());

                gaofs::rpc::remove_data::input in(path);

                handles.emplace_back(ld_network_service->post<gaofs::rpc::remove_data>(endp, in));
            } catch(const exception& e) {
                LOG(ERROR,
                    "Failed to forward non-blocking rpc request to host: {}",endp.to_string());
                return EBUSY;
            }
        }
    }

    //等待RPC回应
    auto err = 0;
    for(const auto& h : handles) {
        try {
            // XXX We might need a timeout here to not wait forever for an
            // output that never comes?
            auto out = h.get().at(0);

            if(out.err() != 0) {
                LOG(ERROR, "received error response: {}", out.err());
                err = out.err();
            }
        } catch(const std::exception& e) {
            LOG(ERROR, "while getting rpc output");
            err = EBUSY;
        }
    }
    return err;
}

// 发送RPC请求以接收目录的所有条目。
pair<int, shared_ptr<gaofs::filemap::OpenDir>> forward_get_dirents(const string& path) {
    LOG(DEBUG, "{}() enter for path '{}'", __func__, path);

    auto const targets = CTX->distributor()->locate_directory_metadata(path);
    // 给目录条目缓冲区分配空间
    auto large_buffer = unique_ptr<char[]>(new char[gaofs::config::rpc::dirents_buff_size]);
    // 每个结点目录条目缓冲区的大小
    const  size_t per_host_buff_size = gaofs::config::rpc::dirents_buff_size / targets.size();

    // 暴露内存
    std::vector<hermes::exposed_memory> exposed_buffers;
    // 内存分配
    exposed_buffers.reserve(targets.size());
    // 暴露内存
    for(size_t i = 0; i < targets.size(); i++) {
        try {
            exposed_buffers.emplace_back(ld_network_service->expose(std::vector<hermes::mutable_buffer>{hermes::mutable_buffer{
                large_buffer.get() + (i * per_host_buff_size), per_host_buff_size
            }}, hermes::access_mode::write_only));
        } catch (const exception& e) {
            LOG(ERROR, "{}() Failed to expose buffers for RMA. err '{}'",
                __func__, e.what());
            return make_pair(EBUSY, nullptr);
        }
    }

    auto err = 0;
    std::vector<hermes::rpc_handle<gaofs::rpc::get_dirents>> handles;

    // 发送rpc
    for(size_t i = 0; i < targets.size(); i++) {
        auto endp = CTX->hosts().at(targets[i]);

        gaofs::rpc::get_dirents::input in(path, exposed_buffers[i]);

        try {
            LOG(DEBUG, "{}() Sending RPC to host: '{}'", __func__, targets[i]);
            handles.emplace_back(ld_network_service->post<gaofs::rpc::get_dirents>(endp, in));
        } catch(const exception& e) {
            LOG(ERROR,
                "{}() Unable to send non-blocking get_dirents() on {} [peer: {}] err '{}'",
                __func__, path, targets[i], e.what());
            err = EBUSY;
            break; // 不直接返回是因为还需要整合response
        }
    }

    LOG(DEBUG,
        "{}() path '{}' send rpc_srv_get_dirents() rpc to '{}' targets. per_host_buff_size '{}' Waiting on reply next and deserialize",
        __func__, path, targets.size(), per_host_buff_size);

    auto send_error = (err != 0);
    auto open_dir = make_shared<gaofs::filemap::OpenDir>(path);

    // 等待rpc返回值
    for(size_t i = 0; i < handles.size(); i++) {
        gaofs::rpc::get_dirents::output out;

        try {
            out = handles[i].get().at(0);

            // 如果在发送过程中出现错误，则跳过直接数据的处理。在本例中，收集了所有响应，但跳过了它们的内容
            if(send_error)
                continue;
            if(out.err() != 0) {
                LOG(ERROR,
                    "{}() Failed to retrieve dir entries from host '{}'. Error '{}', path '{}'",
                    __func__, targets[i], strerror(out.err()), path);
                err = out.err();
                // We need to gather all responses before exiting
                continue;
            }
        } catch(const exception& e) {
            LOG(ERROR,
                "{}() Failed to get rpc output.. [path: {}, target host: {}] err '{}'",
                __func__, path, targets[i], e.what());
            err = EBUSY;
            // We need to gather all responses before exiting
            continue;
        }

        // 每个服务器将信息写入其在large_buffer中预定义的区域，通过计算每个特定服务器的base_address并添加适当的偏移量来恢复信息
        // 就是name 和 bool，bool是type，代表是文件还是目录
        assert(exposed_buffers[i].count() == 1);
        void* base_ptr = exposed_buffers[i].begin()->data();
        bool* bool_ptr = reinterpret_cast<bool*>(base_ptr);
        char* names_ptr = reinterpret_cast<char*>(base_ptr) + (out.dirents_size() * sizeof(bool));

        for(size_t j = 0; j < out.dirents_size(); j++) {
            gaofs::filemap::FileType ftype = (*bool_ptr)? gaofs::filemap::FileType::directory
                                                        : gaofs::filemap::FileType::regular;
            bool_ptr++;

            assert((names_ptr - reinterpret_cast<char*>(base_ptr)) > 0);
            assert(static_cast<unsigned long int>(
                           names_ptr - reinterpret_cast<char*>(base_ptr)) <
                   per_host_buff_size);

            auto name = std::string(names_ptr);
            // number of characters in entry + \0 terminator
            names_ptr += name.size() + 1;

            open_dir->add(name, ftype);
        }
    }
    return make_pair(err, open_dir);
}

// 返回一个具有path-isdir-size和ctime的元组
// 和上一个函数差不多，只不过是对单一服务器来说
pair<int, vector<tuple<const std::string, bool, size_t , time_t>>>
forward_get_dirents_single(const string& path, int server) {
    LOG(DEBUG, "{}() enter for path '{}'", __func__, path);

    auto const targets = CTX->distributor()->locate_directory_metadata(path);

    auto large_buffer = std::unique_ptr<char[]>(new char[gaofs::config::rpc::dirents_buff_size]);

    // 用最大的size对每一个服务器
    const size_t per_host_buff_size = gaofs::config::rpc::dirents_buff_size;
    vector<tuple<const std::string, bool, size_t, time_t>> output;

    // 暴露内存
    std::vector<hermes::exposed_memory> exposed_buffers;
    exposed_buffers.reserve(1);
    size_t i = server;
    try {
        exposed_buffers.emplace_back(ld_network_service->expose(
                std::vector<hermes::mutable_buffer>{hermes::mutable_buffer{
                        large_buffer.get(), per_host_buff_size}},
                hermes::access_mode::write_only));
    } catch(const exception& e) {
        LOG(ERROR, "{}() Failed to expose buffers for RMA. err '{}'", __func__,
            e.what());
        return make_pair(EBUSY, output);
    }

    auto err = 0;
    // 发送rpc
    std::vector<hermes::rpc_handle<gaofs::rpc::get_dirents_extended>> handles;
    auto endp = CTX->hosts().at(targets[i]);
    gaofs::rpc::get_dirents_extended::input in(path, exposed_buffers[0]);

    try {
        LOG(DEBUG, "{}() Sending RPC to host: '{}'", __func__, targets[i]);
        handles.emplace_back(
                ld_network_service->post<gaofs::rpc::get_dirents_extended>(endp,
                                                                          in));
    } catch(const std::exception& ex) {
        LOG(ERROR,
            "{}() Unable to send non-blocking get_dirents() on {} [peer: {}] err '{}'",
            __func__, path, targets[i], ex.what());
        err = EBUSY;
    }

    LOG(DEBUG,
        "{}() path '{}' send rpc_srv_get_dirents() rpc to '{}' targets. per_host_buff_size '{}' Waiting on reply next and deserialize",
        __func__, path, targets.size(), per_host_buff_size);

    // wait for RPC responses

    gaofs::rpc::get_dirents_extended::output out;

    try {
        // XXX We might need a timeout here to not wait forever for an
        // output that never comes?
        out = handles[0].get().at(0);
        // skip processing dirent data if there was an error during send
        // In this case all responses are gathered but their contents skipped

        if(out.err() != 0) {
            LOG(ERROR,
                "{}() Failed to retrieve dir entries from host '{}'. Error '{}', path '{}'",
                __func__, targets[0], strerror(out.err()), path);
            err = out.err();
            // We need to gather all responses before exiting
        }
    } catch(const std::exception& e) {
        LOG(ERROR,
            "{}() Failed to get rpc output.. [path: {}, target host: {}] err '{}'",
            __func__, path, targets[0], e.what());
        err = EBUSY;
        // We need to gather all responses before exiting
    }

    // The parenthesis is extremely important if not the cast will add as a
    // size_t or a time_t and not as a char
    // 将读取的信息回复，并且返回
    auto out_buff_ptr = static_cast<char*>(exposed_buffers[0].begin()->data());
    auto bool_ptr = reinterpret_cast<bool*>(out_buff_ptr);
    auto size_ptr = reinterpret_cast<size_t*>(
            (out_buff_ptr) + (out.dirents_size() * sizeof(bool)));
    auto ctime_ptr = reinterpret_cast<time_t*>(
            (out_buff_ptr) +
            (out.dirents_size() * (sizeof(bool) + sizeof(size_t))));
    auto names_ptr =
            out_buff_ptr + (out.dirents_size() *
                            (sizeof(bool) + sizeof(size_t) + sizeof(time_t)));

    for(std::size_t j = 0; j < out.dirents_size(); j++) {

        bool ftype = (*bool_ptr);
        bool_ptr++;

        size_t size = *size_ptr;
        size_ptr++;

        time_t ctime = *ctime_ptr;
        ctime_ptr++;

        auto name = std::string(names_ptr);
        // number of characters in entry + \0 terminator
        names_ptr += name.size() + 1;
        output.emplace_back(std::forward_as_tuple(name, ftype, size, ctime));
    }
    return make_pair(err, output);

}

// HAS_SYMLINKS
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
                        ->post<gaofs::rpc::mk_symlink>(endp, path, target_path)
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
