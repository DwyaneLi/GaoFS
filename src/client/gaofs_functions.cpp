//
// Created by DELL on 2023/3/26.
//
#include <client/gaofs_functions.hpp>
#include <config.hpp>
#include <client/preload_util.hpp>
#include <client/logging.hpp>
#include <client/rpc/forward_metadata.hpp>
#include <client/rpc/forward_data.hpp>
#include <client/open_dir.hpp>

#include <common/path_util.hpp>

extern "C" {
#include <dirent.h>
#include <linux/kernel.h>
#include <sys/statfs.h>
#include <sys/statvfs.h>
#include <sys/uio.h>
}

using namespace std;

/*
 * Macro used within getdents{,64} functions.
 * __ALIGN_KERNEL defined in linux/kernel.h
 */
#define ALIGN(x, a) __ALIGN_KERNEL((x), (a))

/*
 * linux_dirent is used in getdents() but is privately defined in the linux
 * kernel: fs/readdir.c.
 */
struct linux_dirent {
    unsigned long d_ino;
    unsigned long d_off;
    unsigned short d_reclen;
    char d_name[1];
};
/*
 * linux_dirent64 is used in getdents64() and defined in the linux kernel:
 * include/linux/dirent.h. However, it is not part of the kernel-headers and
 * cannot be imported.
 */
struct linux_dirent64 {
    uint64_t d_ino;
    int64_t d_off;
    unsigned short d_reclen;
    unsigned char d_type;
    char d_name[1]; // originally `char d_name[0]` in kernel, but ISO C++
    // forbids zero-size array 'd_name'
};

struct dirent_extended {
    size_t size;
    time_t ctime;
    unsigned short d_reclen;
    unsigned char d_type;
    char d_name[1]; // originally `char d_name[0]` in kernel, but ISO C++
    // forbids zero-size array 'd_name'
};

namespace {
int check_parent_dir(const string& path) {
#if CREATE_CHECK_PARENTS
    auto p_comp = gaofs::path::dirname(path);
    auto md = gaofs::utils::get_metadata(p_comp);
    if(!md) {
        if(errno == ENOENT) {
            LOG(DEBUG, "Parent component does not exist: '{}'", p_comp);
        } else {
            LOG(ERROR, "Failed to get metadata for parent component '{}': {}",
                path, strerror(errno));
        }
        return -1;
    }
    if(!S_ISDIR(md->mode())) {
        LOG(DEBUG, "Parent component is not a directory: '{}'", p_comp);
        errno = ENOTDIR;
        return -1;
    }
#endif
    return 0;
}

} // namespace

namespace gaofs::syscall {

int gaofs_open(const std::string& path, mode_t mode, int flags) {
    if(flags & O_PATH) {
        LOG(ERROR, "`O_PATH` flag is not supported");
        errno = ENOTSUP;
        return -1;
    }

    if(flags & O_APPEND) {
        LOG(ERROR, "`O_APPEND` flag is not supported");
        errno = ENOTSUP;
        return -1;
    }
    // 在创建或统计期间填充的元数据对象
    gaofs::metadata::Metadata md{};
    // O_CREAT: 文件存在则使用，不存在则新建
    // O_EXCL：检查文件是否存在，不存在则新建，存在则返回错误信息
    // O_CREAT 若此文件不存在则创建它。使用此选项时需要提供第三个参数mode,表示该文件的访问权限。
    // O_EXCL 如果同时指定了O_CREAT,并且文件已存在,则出错返回。
    if(flags & O_CREAT) {
        if(flags & O_DIRECTORY) {
            LOG(ERROR, "O_DIRECTORY use with O_CREAT. NOT SUPPORTED");
            errno = ENOTSUP;
            return -1;
        }
        // no access check required here. If one is using our FS they have the
        // permissions.
        // 没有设置访问权限,gaofs不需要
        auto err = gaofs_create(path, mode | S_IFREG);
        // 用gaofs_create来判断文件是否存在
        if(err) {
            if(errno == EEXIST) {
                // file exists, O_CREAT was set
                if(flags & O_EXCL) {
                    // File exists and O_EXCL & O_CREAT was set
                    return -1;
                }
                // file exists, O_CREAT was set O_EXCL wasnt, so function does
                // not fail this case is actually undefined as per `man 2 open`
                auto md_ = gaofs::utils::get_metadata(path);
                if(!md_) {
                    LOG(ERROR,
                        "Could not get metadata after creating file '{}': '{}'",
                        path, strerror(errno));
                    return -1;
                }
                md = *md_;
            } else {
                LOG(ERROR, "Error creating file: '{}'", strerror(errno));
                return -1;
            }
        } else {
            // 新创建的直接加入到filemap里返回就可以
            return CTX->file_map()->add(
                    std::make_shared<gaofs::filemap::OpenFile>(path, flags));
        }
    } else {
        auto md_ = gaofs::utils::get_metadata(path);
        if(!md_) {
            if(errno != ENOENT) {
                LOG(ERROR, "Error stating existing file '{}'", path);
            }
            // file doesn't exists and O_CREAT was not set
            return -1;
        }
        md = *md_;
    }
// SYMLINKS
#ifdef HAS_SYMLINKS
    if(md.is_link()) {
        if(flags & O_NOFOLLOW) {
            LOG(WARNING, "Symlink found and O_NOFOLLOW flag was specified");
            errno = ELOOP;
            return -1;
        }
        return gaofs_open(md.target_path(), mode, flags);
    }
#endif

    // 如果是目录，则打开目录
    if(S_ISDIR(md.mode())) {
        return gaofs_opendir(path);
    }

    // 对于之前就已经存在的普通文件的处理
    assert(S_ISREG(md.mode()));

    if((flags & O_TRUNC) && ((flags & O_RDWR) || (flags & O_WRONLY))) {
        if(gaofs_truncate(path, md.size(), 0)) {
            LOG(ERROR, "Error truncating file");
            return -1;
        }
    }

    return CTX->file_map()->add(
            std::make_shared<gaofs::filemap::OpenFile>(path, flags));
}

int gaofs_create(const string& path, mode_t mode) {
    //只支持目录和文件
    switch(mode & S_IFMT) {
        case 0:
            mode |= S_IFREG;
            break;
        case S_IFREG: // intentionally fall-through
        case S_IFDIR:
            break;
        case S_IFCHR: // intentionally fall-through
        case S_IFBLK:
        case S_IFIFO:
        case S_IFSOCK:
            LOG(WARNING, "Unsupported node type");
            errno = ENOTSUP;
            return -1;
        default:
            LOG(WARNING, "Unrecognized node type");
            errno = EINVAL;
            return -1;
    }
    // 检查是否存在父目录
    if(check_parent_dir(path)) {
        return -1;
    }
    auto err = gaofs::rpc::forward_create(path, mode);
    if(err) {
        errno = err;
        return -1;
    }
    return 0;
}

// 删除文件，无法删除目录
int gaofs_remove(const std::string& path) {
    auto md = gaofs::utils::get_metadata(path);
    if(!md) {
        return -1;
    }
    if(S_ISDIR(md->mode())) {
        LOG(ERROR, "Cannot remove directory '{}'", path);
        errno = EISDIR;
        return -1;
    }

    auto err = gaofs::rpc::forward_remove(path);
    if(err) {
        errno = err;
        return -1;
    }
    return 0;
}

// 检查函数是否可读写
int gaofs_access(const std::string& path, const int mask, bool follow_links) {
    auto md = gaofs::utils::get_metadata(path, follow_links);
    if(!md) {
        return -1;
    }
    return 0;
}

int gaofs_stat(const string& path, struct stat* buf, bool follow_links) {
    auto md = gaofs::utils::get_metadata(path, follow_links);
    if(!md) {
        return -1;
    }
    gaofs::utils::metadata_to_stat(path, *md, *buf);
    return 0;
}

// TODO: statx_type
#ifdef STATX_TYPE
int gaofs_statx(int dirfs, const std::string& path, int flags, unsigned int mask,
           struct statx* buf, bool follow_links) {
    auto md = gaofs::utils::get_metadata(path, follow_links);
    if(!md) {
        return -1;
    }

    struct stat tmp {};

    gaofs::utils::metadata_to_stat(path, *md, tmp);

    buf->stx_mask = 0;
    buf->stx_blksize = tmp.st_blksize;
    buf->stx_attributes = 0;
    buf->stx_nlink = tmp.st_nlink;
    buf->stx_uid = tmp.st_uid;
    buf->stx_gid = tmp.st_gid;
    buf->stx_mode = tmp.st_mode;
    buf->stx_ino = tmp.st_ino;
    buf->stx_size = tmp.st_size;
    buf->stx_blocks = tmp.st_blocks;
    buf->stx_attributes_mask = 0;

    buf->stx_atime.tv_sec = tmp.st_atim.tv_sec;
    buf->stx_atime.tv_nsec = tmp.st_atim.tv_nsec;

    buf->stx_mtime.tv_sec = tmp.st_mtim.tv_sec;
    buf->stx_mtime.tv_nsec = tmp.st_mtim.tv_nsec;

    buf->stx_ctime.tv_sec = tmp.st_ctim.tv_sec;
    buf->stx_ctime.tv_nsec = tmp.st_ctim.tv_nsec;

    buf->stx_btime = buf->stx_atime;

    return 0;
}
#endif

// 查看文件系统的信息，保存在buf里
int gaofs_statfs(struct statfs* buf) {
    auto ret = gaofs::rpc::forward_get_chunk_stat();
    auto err = ret.first;
    if(err) {
        LOG(ERROR, "{}() Failure with error: '{}'", err);
        errno = err;
        return -1;
    }
    auto blk_stat = ret.second;
    buf->f_type = 0;
    buf->f_bsize = blk_stat.chunk_size;
    buf->f_blocks = blk_stat.chunk_total;
    buf->f_bfree = blk_stat.chunk_free;
    buf->f_bavail = blk_stat.chunk_free;
    buf->f_files = 0;
    buf->f_ffree = 0;
    buf->f_fsid = {0, 0};
    buf->f_namelen = path::max_length;
    buf->f_frsize = 0;
    buf->f_flags =
            ST_NOATIME | ST_NODIRATIME | ST_NOSUID | ST_NODEV | ST_SYNCHRONOUS;
    return 0;
}

// todo: GAOFS_ENABLE_UNUSED_FUNCTIONS
#ifdef GAOFS_ENABLE_UNUSED_FUNCTIONS
int gaofs_statvfs(struct statvfs* buf) {
    auto ret = gaofs::rpc::forward_get_chunk_stat();
    auto err = ret.first;
    if(err) {
        LOG(ERROR, "{}() Failure with error: '{}'", err);
        errno = err;
        return -1;
    }
    auto blk_stat = ret.second;
    buf->f_bsize = blk_stat.chunk_size;
    buf->f_blocks = blk_stat.chunk_total;
    buf->f_bfree = blk_stat.chunk_free;
    buf->f_bavail = blk_stat.chunk_free;
    buf->f_files = 0;
    buf->f_ffree = 0;
    buf->f_favail = 0;
    buf->f_fsid = 0;
    buf->f_namemax = path::max_length;
    buf->f_frsize = 0;
    buf->f_flag =
            ST_NOATIME | ST_NODIRATIME | ST_NOSUID | ST_NODEV | ST_SYNCHRONOUS;
    return 0;
}
#endif

// 使用 lseek 函数可以改变文件的 cfo 使用文件描述符
off_t gaofs_lseek(unsigned int fd, off_t offset, unsigned int whence) {
    return gaofs_lseek(CTX->file_map()->get(fd), offset, whence);
}

off_t gaofs_lseek(shared_ptr<gaofs::filemap::OpenFile> gaofs_fd, off_t offset, unsigned int whence) {
    switch(whence) {
        // 文件偏移量被设置成offset
        case SEEK_SET:
            if(offset < 0) {
                errno = EINVAL;
                return -1;
            }
            gaofs_fd->pos(offset);
            break;
            // 文件偏移量+=offset
        case SEEK_CUR:
            gaofs_fd->pos(gaofs_fd->pos() + offset);
            break;
            // 文件偏移量将被设置为文件长度加上 offset，offset 可以为正也可以为负。
        case SEEK_END: {
            auto ret = gaofs::rpc::forward_get_metadentry_size(gaofs_fd->path());
            auto err = ret.first;
            if(err) {
                errno = err;
                return -1;
            }

            auto file_size = ret.second;
            if(offset < 0 && file_size < -offset) {
                errno = EINVAL;
                return -1;
            }
            gaofs_fd->pos(file_size + offset);
            break;
        }
        case SEEK_DATA:
            LOG(WARNING, "SEEK_DATA whence is not supported");
            // We do not support this whence yet
            errno = EINVAL;
            return -1;
        case SEEK_HOLE:
            LOG(WARNING, "SEEK_HOLE whence is not supported");
            // We do not support this whence yet
            errno = EINVAL;
            return -1;
        default:
            LOG(WARNING, "Unknown whence value {:#x}", whence);
            errno = EINVAL;
            return -1;
    }
    return gaofs_fd->pos();
}

// 截断操作
int gaofs_truncate(const std::string& path, off_t old_size, off_t new_size) {
    assert(new_size >= 0);
    assert(new_size <= old_size);

    if(new_size == old_size) {
        return 0;
    }

    auto err = gaofs::rpc::forward_decr_size(path, new_size);
    if(err) {
        LOG(DEBUG, "Failed to decrease size");
        errno = err;
        return -1;
    }
    // 再截断
    err = gaofs::rpc::forward_truncate(path, old_size, new_size);
    if(err) {
        LOG(DEBUG, "Failed to truncate data");
        errno = err;
        return -1;
    }
    return 0;
}

// 截断操作
int gaofs_truncate(const std::string& path, off_t offset) {
    if(offset < 0) {
        LOG(DEBUG, "Length is negative: {}", offset);
        errno = EINVAL;
        return -1;
    }

    auto md = gaofs::utils::get_metadata(path, true);
    if(!md) {
        return -1;
    }

    auto size = md->size();
    if(static_cast<unsigned long>(offset) > size) {
        LOG(DEBUG, "Length is greater then file size: {} > {}", offset, size);
        errno = EINVAL;
        return -1;
    }
    return gaofs_truncate(path, size, offset);
}

// 复制一个现存的文件描述符 dup
int gaofs_dup(const int oldfd) {
    return CTX->file_map()->dup(oldfd);
}

// 复制一个现存的文件描述符 dup2
int gaofs_dup2(const int oldfd, const int newfd) {
    return CTX->file_map()->dup2(oldfd, newfd);
}

// 打开目录
int gaofs_opendir(const string& path) {
    auto md = gaofs::utils::get_metadata(path);
    if(!md) {
        return -1;
    }

    if(!S_ISDIR(md->mode())) {
        LOG(DEBUG, "Path is not a directory");
        errno = ENOTDIR;
        return -1;
    }

    auto ret = gaofs::rpc::forward_get_dirents(path);
    auto err = ret.first;
    if(err) {
        errno = err;
        return -1;
    }
    assert(ret.second);
    return CTX->file_map()->add(ret.second);

}

// 删除目录
int gaofs_rmdir(const string& path) {
    auto md = gaofs::utils::get_metadata(path);
    if(!md) {
        LOG(DEBUG, "Error: Path '{}' err code '{}' ", path, strerror(errno));
        return -1;
    }
    if(!S_ISDIR(md->mode())) {
        LOG(DEBUG, "Path '{}' is not a directory", path);
        errno = ENOTDIR;
        return -1;
    }

    auto ret = gaofs::rpc::forward_get_dirents(path);
    auto err = ret.first;
    if(err) {
        errno = err;
        return -1;
    }
    assert(ret.second);
    auto open_dir = ret.second;
    // 目录里有东西就不能删，必须是空目录
    if(open_dir->size() != 0) {
        errno = ENOTEMPTY;
        return -1;
    }
    err = gaofs::rpc::forward_remove(path);
    if(err) {
        errno = err;
        return -1;
    }
    return 0;
}

// 获取目录项
int gaofs_getdents(unsigned int fd, struct linux_dirent* dirp, unsigned int count) {
    // 获取opendir对象
    auto open_dir = CTX->file_map()->get_dir(fd);
    if(open_dir == nullptr) {
        errno = EBADF;
        return -1;
    }

    // get directory position of which entries to return
    auto pos = open_dir->pos();
    if(pos >= open_dir->size()) {
        return 0;
    }

    unsigned int written = 0;
    struct linux_dirent* current_dirp = nullptr;
    while(pos < open_dir->size()) {
        // 获取当前条目
        auto de = open_dir->getdent(pos);
        // 根据文件名大小计算内核结构体`linux_dirent`中的总dentry大小。
        // 然后将大小对齐到`long`边界的大小。
        auto total_size = ALIGN(offsetof(struct linux_dirent, d_name) +
                                        de.name().size() + 3,
                                sizeof(long));

        if(total_size > (count - written)) {
            break;
        }

        // 填写对应dirent结构成员
        current_dirp = reinterpret_cast<struct linux_dirent*>(reinterpret_cast<char*>(dirp) + written);
        current_dirp->d_ino =std::hash<std::string>()(open_dir->path() + "/" + de.name());
        current_dirp->d_reclen = total_size;
        *(reinterpret_cast<char*>(current_dirp) + total_size - 1) =
                ((de.type() == gaofs::filemap::FileType::regular) ? DT_REG : DT_DIR);

        LOG(DEBUG, "name {}: {}", pos, de.name());
        std::strcpy(&(current_dirp->d_name[0]), de.name().c_str());
        pos++;
        current_dirp->d_off = pos;
        written += total_size;
    }

    if(written == 0) {
        errno = EINVAL;
        return -1;
    }

    open_dir->pos(pos);
    return written;
}

int gaofs_getdents64(unsigned int fd, struct linux_dirent64* dirp,
                unsigned int count) {

    auto open_dir = CTX->file_map()->get_dir(fd);
    if(open_dir == nullptr) {
        // Cast did not succeeded: open_file is a regular file
        errno = EBADF;
        return -1;
    }

    auto pos = open_dir->pos();
    if(pos >= open_dir->size()) {
        return 0;
    }

    unsigned int written = 0;
    struct linux_dirent64* current_dirp = nullptr;
    while(pos < open_dir->size()) {
        auto de = open_dir->getdent(pos);
        // 根据文件名大小计算内核结构体`linux_dirent`中的总dentry大小。
        // 然后将大小对齐到`long`边界的大小。
        auto total_size = ALIGN(offsetof(struct linux_dirent64, d_name) +
                                        de.name().size() + 1,
                                sizeof(uint64_t));
        if(total_size > (count - written)) {
            break;
        }

        current_dirp = reinterpret_cast<struct linux_dirent64*>(reinterpret_cast<char*>(dirp) + written);
        current_dirp->d_ino =
                std::hash<std::string>()(open_dir->path() + "/" + de.name());

        current_dirp->d_reclen = total_size;
        current_dirp->d_type =
                ((de.type() == gaofs::filemap::FileType::regular) ? DT_REG : DT_DIR);

        LOG(DEBUG, "name {}: {}", pos, de.name());
        std::strcpy(&(current_dirp->d_name[0]), de.name().c_str());
        ++pos;
        current_dirp->d_off = pos;
        written += total_size;
    }

    if(written == 0) {
        errno = EINVAL;
        return -1;
    }
    open_dir->pos(pos);
    return written;
}

// pwrite
ssize_t gaofs_pwrite(std::shared_ptr<gaofs::filemap::OpenFile> file, const char* buf,
            size_t count, off64_t offset) {
    if(file->type() != gaofs::filemap::FileType::regular) {
        assert(file->type() == gaofs::filemap::FileType::directory);
        LOG(WARNING, "Cannot read from directory");
        errno = EISDIR;
        return -1;
    }
    // 获取文件路径
    auto path = make_shared<string>(file->path());
    auto append_flag = file->get_flag(gaofs::filemap::OpenFile_flags::append);
    // 更新文件新的大小
    auto ret_update_size = gaofs::rpc::forward_update_metadentry_size(*path, count, offset, append_flag);
    auto err = ret_update_size.first;
    if(err) {
        LOG(ERROR, "update_metadentry_size() failed with err '{}'", err);
        errno = err;
        return -1;
    }

    auto updated_size = ret_update_size.second;
    // 进行写操作
    auto ret_write = gaofs::rpc::forward_write(*path, buf, append_flag, offset,
                                                  count, updated_size);
    err = ret_write.first;
    if(err) {
        LOG(WARNING, "gaofs::rpc::forward_write() failed with err '{}'", err);
        errno = err;
        return -1;
    }
    return ret_write.second; // return written size
}

// 对pwrite的封装，pwrite是通过path，ws是通过fd
ssize_t gaofs_pwrite_ws(int fd, const void* buf, size_t count, off64_t offset) {
    auto file = CTX->file_map()->get(fd);
    return gaofs_pwrite(file, reinterpret_cast<const char*>(buf), count, offset);
}

//在当前pos后写，追加写，会改变pos位置，但是pwrite不会改变
// todo: pos获取的位置有问题？
ssize_t gaofs_write(int fd, const void* buf, size_t count) {
    auto file = CTX->file_map()->get(fd);
    auto pos = file->pos();
    // 追加写要把pos切换到文件末尾
    if(file->get_flag(gaofs::filemap::OpenFile_flags::append)) {
        gaofs_lseek(file, 0, SEEK_END);
    }

    // 使用pwrite进行文件写
    auto ret = gaofs_pwrite(file, reinterpret_cast<const char*>(buf), count, pos);
    // 更新pos位置
    if(ret > 0) {
        file->pos(pos + count);
    }
    return ret;
}

// 将多个缓冲区的数据写在一起
ssize_t gaofs_pwritev(int fd, const struct iovec* iov, int iovcnt, off_t offset) {
    auto file = CTX->file_map()->get(fd);
    auto pos = offset;
    ssize_t written = 0;
    ssize_t ret;
    // 对各个缓冲区进行操作
    for(int i = 0; i < iovcnt; i++) {
        auto count = (iov + i)->iov_len;
        if(count == 0) {
            continue;
        }
        auto buf = (iov + i)->iov_base;
        ret = gaofs_pwrite(file, reinterpret_cast<char*>(buf), count, pos);
        if(ret == -1) {
            break;
        }
        written += ret;
        pos += ret;

        // todo：为什么是直接break？
        if(static_cast<size_t>(ret) < count) {
            break;
        }
    }
    if(written == 0) {
        return -1;
    }
    return written;
}

ssize_t gaofs_writev(int fd, const struct iovec* iov, int iovcnt) {
    auto file = CTX->file_map()->get(fd);
    auto pos = file->pos(); // retrieve the current offset
    auto ret = gaofs_pwritev(fd, iov, iovcnt, pos);
    assert(ret != 0);
    if(ret < 0) {
        return -1;
    }
    file->pos(pos + ret);
    return ret;
}

ssize_t gaofs_pread(std::shared_ptr<gaofs::filemap::OpenFile> file, char* buf,
           size_t count, off64_t offset) {
    if(file->type() != gaofs::filemap::FileType::regular) {
        assert(file->type() == gaofs::filemap::FileType::directory);
        LOG(WARNING, "Cannot read from directory");
        errno = EISDIR;
        return -1;
    }

    // 清空缓冲区并且分配空间
    if constexpr(gaofs::config::io::zero_buffer_before_read) {
        memset(buf, 0, sizeof(char) * count);
    }
    auto ret = gaofs::rpc::forward_read(file->path(), buf, offset, count);
    auto err = ret.first;
    if(err) {
        LOG(WARNING, "gaofs::rpc::forward_read() failed with ret '{}'", err);
        errno = err;
        return -1;
    }
    return ret.second;
}

ssize_t gaofs_pread_ws(int fd, void* buf, size_t count, off64_t offset) {
    auto gaofs_fd = CTX->file_map()->get(fd);
    return gaofs_pread(gaofs_fd, reinterpret_cast<char*>(buf), count, offset);
}

// 与上一个函数类似，不过需要更新pos
ssize_t gaofs_read(int fd, void* buf, size_t count) {
    auto gaofs_fd = CTX->file_map()->get(fd);
    auto pos = gaofs_fd->pos(); // retrieve the current offset
    auto ret = gaofs_pread(gaofs_fd, reinterpret_cast<char*>(buf), count, pos);
    // Update offset in file descriptor in the file map
    // 读完之后要更新pos
    if(ret > 0) {
        gaofs_fd->pos(pos + ret);
    }
    return ret;
}


// 将一段内容读入多个缓冲区
ssize_t gaofs_preadv(int fd, const struct iovec* iov, int iovcnt, off_t offset) {
    auto file = CTX->file_map()->get(fd);
    auto pos = offset; // keep track of current position
    ssize_t read = 0;
    ssize_t ret;
    for(int i = 0; i < iovcnt; ++i) {
        auto count = (iov + i)->iov_len;
        if(count == 0) {
            continue;
        }
        auto buf = (iov + i)->iov_base;
        ret = gaofs_pread(file, reinterpret_cast<char*>(buf), count, pos);
        if(ret == -1) {
            break;
        }
        read += ret;
        pos += ret;

        if(static_cast<size_t>(ret) < count) {
            break;
        }
    }

    if(read == 0) {
        return -1;
    }
    return read;
}

// 与上一个类似，只不过要改pos
ssize_t gaofs_readv(int fd, const struct iovec* iov, int iovcnt) {

    auto gaofs_fd = CTX->file_map()->get(fd);
    auto pos = gaofs_fd->pos(); // retrieve the current offset
    auto ret = gaofs_preadv(fd, iov, iovcnt, pos);
    assert(ret != 0);
    if(ret < 0) {
        return -1;
    }
    gaofs_fd->pos(pos + ret);
    return ret;
}


#ifdef HAS_SYMLINKS
#ifdef GAOFS_ENABLE_UNUSED_FUNCTIONS
/**
 * gaofs wrapper for make symlink() system calls
 * errno may be set
 *
 * * NOTE: Currently unused
 *
 * @param path
 * @param target_path
 * @return 0 on success or -1 on error
 */
int
gaofs_mk_symlink(const std::string& path, const std::string& target_path) {
    /* The following check is not POSIX compliant.
     * In POSIX the target is not checked at all.
     *  Here if the target is a directory we raise a NOTSUP error.
     *  So that application know we don't support link to directory.
     */
    auto target_md = gaofs::utils::get_metadata(target_path, false);
    if(target_md) {
        auto trg_mode = target_md->mode();
        if(!(S_ISREG(trg_mode) || S_ISLNK(trg_mode))) {
            assert(S_ISDIR(trg_mode));
            LOG(DEBUG, "Target path is a directory. Not supported");
            errno = ENOTSUP;
            return -1;
        }
    }

    if(check_parent_dir(path)) {
        return -1;
    }

    auto link_md = gaofs::utils::get_metadata(path, false);
    if(link_md) {
        LOG(DEBUG, "Link exists: '{}'", path);
        errno = EEXIST;
        return -1;
    }

    auto err = gaofs::rpc::forward_mk_symlink(path, target_path);
    if(err) {
        errno = err;
        return -1;
    }
    return 0;
}

/**
 * gaofs wrapper for reading symlinks
 * errno may be set
 *
 * NOTE: Currently unused
 *
 * @param path
 * @param buf
 * @param bufsize
 * @return 0 on success or -1 on error
 */
int
gaofs_readlink(const std::string& path, char* buf, int bufsize) {
    auto md = gaofs::utils::get_metadata(path, false);
    if(!md) {
        LOG(DEBUG, "Named link doesn't exist");
        return -1;
    }
    if(!(md->is_link())) {
        LOG(DEBUG, "The named file is not a symbolic link");
        errno = EINVAL;
        return -1;
    }
    int path_size = md->target_path().size() + CTX->mountdir().size();
    if(path_size >= bufsize) {
        LOG(WARNING, "Destination buffer size is too short: {} < {}, {} ",
            bufsize, path_size, md->target_path());
        errno = ENAMETOOLONG;
        return -1;
    }

    CTX->mountdir().copy(buf, CTX->mountdir().size());
    std::strcpy(buf + CTX->mountdir().size(), md->target_path().c_str());
    return path_size;
}
#endif
#endif

} // namespace gaofs::syscall

// 这个函数定义了一个dirent的扩展，准备执行类似查找的函数，该函数只将请求发送到指定的服务器
// 可能可以用于find接口
extern "C" int
gaofs_getsingleserverdir(const char* path, struct dirent_extended* dirp,
                        unsigned int count, int server) {

    auto ret = gaofs::rpc::forward_get_dirents_single(path, server);
    auto err = ret.first;
    if(err) {
        errno = err;
        return -1;
    }

    auto open_dir = ret.second;
    unsigned int pos = 0;
    unsigned int written = 0;
    struct dirent_extended* current_dirp = nullptr;
    while(pos < open_dir.size()) {
        auto de = open_dir[pos];
        /*
         * Calculate the total dentry size within the 'dirent_extended`
         * depending on the file name size. The size is then aligned to the size
         * of `long` boundary.
         */
        auto total_size = ALIGN(offsetof(struct dirent_extended, d_name) +
                                (get<0>(de)).size() + 1,
                                sizeof(uint64_t));
        if(total_size > (count - written)) {
            // no enough space left on user buffer to insert next dirent
            break;
        }
        current_dirp = reinterpret_cast<struct dirent_extended*>(
                reinterpret_cast<char*>(dirp) + written);

        current_dirp->d_reclen = total_size;
        current_dirp->d_type = get<1>(de);
        current_dirp->size = get<2>(de);
        current_dirp->ctime = get<3>(de);

        LOG(DEBUG, "name {}: {} {} {} {} / size {}", pos, get<0>(de),
            get<1>(de), get<2>(de), get<3>(de), total_size);
        std::strcpy(&(current_dirp->d_name[0]), (get<0>(de)).c_str());
        ++pos;
        written += total_size;
    }

    if(written == 0) {
        errno = EINVAL;
        return -1;
    }
    return written;
}
