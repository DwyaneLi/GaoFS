//
// Created by DELL on 2023/3/19.
//

#ifndef GAOFS_FORWARD_MATADATA_HPP
#define GAOFS_FORWARD_MATADATA_HPP

#include <string>
#include <memory>
#include <vector>

namespace gaofs {
namespace filemap {
    class OpenDir;
}

namespace metadata {
    struct MetadentryUpdateFlags;

    class Metadata;
} // namespace metadata

// TODO once we have LEAF, remove all the error code returns and throw them as
// an exception.

namespace rpc {

int forward_create(const std::string& path, mode_t mode);

int forward_stat(const std::string& path, std::string& attr);

int forward_remove(const std::string& path);

int forward_decr_size(const std::string& path, size_t length);

int forward_update_metadentry(const std::string& path, const gaofs::metadata::Metadata& md,
        const gaofs::metadata::MetadentryUpdateFlags& md_flags);

std::pair<int, off64_t> forward_update_metadentry_size(const std::string& path, size_t size,
                               off64_t offset, bool append_flag);

std::pair<int, off64_t> forward_get_metadentry_size(const std::string& path);

std::pair<int, std::shared_ptr<gaofs::filemap::OpenDir>> forward_get_dirents(const std::string& path);

std::pair<int, std::vector<std::tuple<const std::string, bool, size_t, time_t>>>
forward_get_dirents_single(const std::string& path, int server);

#ifdef HAS_SYMLINKS
// TODO: has_symlinks
int forward_mk_symlink(const std::string& path, const std::string& target_path);
#endif

} // namespace rpc

} // namespace gaofs

#endif //GAOFS_FORWARD_MATADATA_HPP
