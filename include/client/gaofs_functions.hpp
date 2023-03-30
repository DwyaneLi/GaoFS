//
// Created by DELL on 2023/3/26.
//

#ifndef GAOFS_GAOFS_FUNCTIONS_HPP
#define GAOFS_GAOFS_FUNCTIONS_HPP

#include <client/open_file_map.hpp>
#include <common/metadata.hpp>

struct statfs;
struct statvfs;
struct linux_dirent;
struct linux_dirent64;
struct iovec;

namespace gaofs::syscall {

int gaofs_open(const std::string& path, mode_t mode, int flags);

int gaofs_create(const std::string& path, mode_t mode);

int gaofs_remove(const std::string& path);

// Implementation of access,
// Follow links is true by default
int gaofs_access(const std::string& path, int mask, bool follow_links = true);

// Implementation of stat,
// Follow links is true by default
int gaofs_stat(const std::string& path, struct stat* buf, bool follow_links = true);

// Implementation of statx, it uses the normal stat and maps the information to
// the statx structure Follow links is true by default
// TODO STATX_TYPE
#ifdef STATX_TYPE

int gaofs_statx(int dirfd, const std::string& path, int flags, unsigned int mask,
           struct statx* buf, bool follow_links = true);

#endif

int gaofs_statfs(struct statfs* buf);

int gaofs_statvfs(struct statvfs* buf);

off64_t gaofs_lseek(unsigned int fd, off64_t offset, unsigned int whence);

off64_t gaofs_lseek(std::shared_ptr<gaofs::filemap::OpenFile> gaofs_fd, off64_t offset,
           unsigned int whence);

int gaofs_truncate(const std::string& path, off_t offset);

int gaofs_truncate(const std::string& path, off_t old_size, off_t new_size);

int gaofs_dup(int oldfd);

int gaofs_dup2(int oldfd, int newfd);

// TODO SYMLINKS
#ifdef HAS_SYMLINKS

int gaofs_mk_symlink(const std::string& path, const std::string& target_path);

int gaofs_readlink(const std::string& path, char* buf, int bufsize);

#endif

ssize_t gaofs_pwrite(std::shared_ptr<gaofs::filemap::OpenFile> file, const char* buf,
            size_t count, off64_t offset);

ssize_t gaofs_pwrite_ws(int fd, const void* buf, size_t count, off64_t offset);

ssize_t gaofs_write(int fd, const void* buf, size_t count);

ssize_t gaofs_pwritev(int fd, const struct iovec* iov, int iovcnt, off_t offset);

ssize_t gaofs_writev(int fd, const struct iovec* iov, int iovcnt);

ssize_t gaofs_pread(std::shared_ptr<gaofs::filemap::OpenFile> file, char* buf,
           size_t count, off64_t offset);

ssize_t gaofs_pread_ws(int fd, void* buf, size_t count, off64_t offset);

ssize_t gaofs_read(int fd, void* buf, size_t count);

ssize_t gaofs_readv(int fd, const struct iovec* iov, int iovcnt);

ssize_t gaofs_preadv(int fd, const struct iovec* iov, int iovcnt, off_t offset);

int gaofs_opendir(const std::string& path);

int gaofs_getdents(unsigned int fd, struct linux_dirent* dirp, unsigned int count);

int gaofs_getdents64(unsigned int fd, struct linux_dirent64* dirp,
                unsigned int count);

int gaofs_rmdir(const std::string& path);

} // namespace gaofs::syscall

extern "C" int gaofs_getsingleserverdir(const char* path, struct dirent_extended* dirp,
                        unsigned int count, int server);

#endif //GAOFS_GAOFS_FUNCTIONS_HPP
