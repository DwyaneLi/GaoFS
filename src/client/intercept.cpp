//
// Created by DELL on 2023/3/29.
//

#include <client/intercept.hpp>
#include <client/preload.hpp>
#include <client/hooks.hpp>
#include <client/logging.hpp>

#include <optional>
#include <fmt/format.h>

#include <cerrno>

extern "C" {
#include <syscall.h>
#include <libsyscall_intercept_hook_point.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <printf.h>
}

namespace {
// thread_local关键字的变量具有线程周期，这些变量在线程开始的时候就被生成，在线程销毁的时候就被销毁，
// 并且每一个线程都具有一个独立的变量,也就是相当于每一个线程都有一份不同的变量
// thread_local相当于通过将可变变量让每一个线程都拥有独立的副本，从而实现线程封装的机制

thread_local bool reentrance_guard_flag; // 再进入的标志
thread_local gaofs::syscall::info saved_syscall_info;

// 对saved_syscall_info进行操作的函数
constexpr void save_current_syscall_info(gaofs::syscall::info info) {
    saved_syscall_info = info;
}

// todo: 为什么会报错
void reset_current_syscall_info() {
    saved_syscall_info = gaofs::syscall::no_info;
}

inline gaofs::syscall::info get_current_syscall_info() {
    return saved_syscall_info;
}

/*
 * 这个钩子基本上用来跟踪库本身内部创建的文件描述符。这一点很重要，
 * 因为一些应用程序(例如ssh)可能试图关闭所有打开的文件描述符，这将使
 * 库内部处于不一致的状态。我们将系统调用转发给内核，但我们会跟踪任何
 * 可能创建或销毁文件描述符的系统调用，以便将它们标记为“内部”。
 */
inline int hook_internal(long syscall_number, long arg0, long arg1, long arg2, long arg3,
                         long arg4, long arg5, long* result) {
// gaofs_enable_logging   GAOFS_DEBUG_BUILD
#if defined(GAOFS_ENABLE_LOGGING) && defined(GAOFS_DEBUG_BUILD)
    const long args[gaofs::syscall::MAX_ARGS] = {arg0, arg1, arg2,
                                                arg3, arg4, arg5};
#endif

    int syserror = 0;
    LOG(SYSCALL,
        gaofs::syscall::from_internal_code | gaofs::syscall::to_hook |
        gaofs::syscall::not_executed,
        syscall_number, args);

    switch(syscall_number) {

        case SYS_open:
            *result = syscall_no_intercept(
                    syscall_number, reinterpret_cast<char*>(arg0),
                    static_cast<int>(arg1), static_cast<mode_t>(arg2));
            syserror = syscall_error_code(*result);

            if(syserror == 0) {
                *result = CTX->register_internal_fd(*result);
            } else {
                *result = -syserror;
            }

            break;

        case SYS_creat:
            *result = syscall_no_intercept(
                    syscall_number, reinterpret_cast<const char*>(arg0),
                    O_WRONLY | O_CREAT | O_TRUNC, static_cast<mode_t>(arg1));
            syserror = syscall_error_code(*result);

            if(syserror == 0) {
                *result = CTX->register_internal_fd(*result);
            } else {
                *result = -syserror;
            }

            break;

        case SYS_openat:
            *result = syscall_no_intercept(
                    syscall_number, static_cast<int>(arg0),
                    reinterpret_cast<const char*>(arg1), static_cast<int>(arg2),
                    static_cast<mode_t>(arg3));
            syserror = syscall_error_code(*result);

            if(syserror == 0) {
                *result = CTX->register_internal_fd(*result);
            } else {
                *result = -syserror;
            }
            break;

        case SYS_epoll_create:
            *result = syscall_no_intercept(syscall_number,
                                           static_cast<int>(arg0));
            syserror = syscall_error_code(*result);

            if(syserror == 0) {
                *result = CTX->register_internal_fd(*result);
            } else {
                *result = -syserror;
            }

            break;

        case SYS_epoll_create1:
            *result = syscall_no_intercept(syscall_number,
                                           static_cast<int>(arg0));
            syserror = syscall_error_code(*result);

            if(syserror == 0) {
                *result = CTX->register_internal_fd(*result);
            } else {
                *result = -syserror;
            }

            break;

        case SYS_dup:
            *result = syscall_no_intercept(syscall_number,
                                           static_cast<unsigned int>(arg0));
            syserror = syscall_error_code(*result);

            if(syserror == 0) {
                *result = CTX->register_internal_fd(*result);
            }

            break;

        case SYS_dup2:
            *result = syscall_no_intercept(syscall_number,
                                           static_cast<unsigned int>(arg0),
                                           static_cast<unsigned int>(arg1));
            syserror = syscall_error_code(*result);

            if(syserror == 0) {
                *result = CTX->register_internal_fd(*result);
            }

            break;

        case SYS_dup3:
            *result = syscall_no_intercept(
                    syscall_number, static_cast<unsigned int>(arg0),
                    static_cast<unsigned int>(arg1), static_cast<int>(arg2));

            syserror = syscall_error_code(*result);

            if(syserror == 0) {


                *result = CTX->register_internal_fd(*result);
            }

            break;

        case SYS_inotify_init:
            *result = syscall_no_intercept(syscall_number);

            if(*result >= 0) {
                *result = CTX->register_internal_fd(*result);
            }

            break;

        case SYS_inotify_init1:
            *result = syscall_no_intercept(syscall_number,
                                           static_cast<int>(arg0));

            if(*result >= 0) {
                *result = CTX->register_internal_fd(*result);
            }

            break;

        case SYS_perf_event_open:
            *result = syscall_no_intercept(
                    syscall_number,
                    reinterpret_cast<struct perf_event_attr*>(arg0),
                    static_cast<pid_t>(arg1), static_cast<int>(arg2),
                    static_cast<int>(arg3), static_cast<unsigned long>(arg4));

            if(*result >= 0) {
                *result = CTX->register_internal_fd(*result);
            }
            break;

        case SYS_signalfd:
            *result = syscall_no_intercept(
                    syscall_number, static_cast<int>(arg0),
                    reinterpret_cast<const sigset_t*>(arg1));

            if(*result >= 0) {
                *result = CTX->register_internal_fd(*result);
            }
            break;

        case SYS_signalfd4:
            *result = syscall_no_intercept(
                    syscall_number, static_cast<int>(arg0),
                    reinterpret_cast<const sigset_t*>(arg1),
                    static_cast<int>(arg2));

            if(*result >= 0) {
                *result = CTX->register_internal_fd(*result);
            }
            break;

        case SYS_timerfd_create:
            *result =
                    syscall_no_intercept(syscall_number, static_cast<int>(arg0),
                                         static_cast<int>(arg1));

            if(*result >= 0) {
                *result = CTX->register_internal_fd(*result);
            }
            break;


        case SYS_socket:
            *result = syscall_no_intercept(
                    syscall_number, static_cast<int>(arg0),
                    static_cast<int>(arg1), static_cast<int>(arg2));

            if(*result >= 0) {
                *result = CTX->register_internal_fd(*result);
            }
            break;

        case SYS_socketpair:

            *result = syscall_no_intercept(
                    syscall_number, static_cast<int>(arg0),
                    static_cast<int>(arg1), static_cast<int>(arg2),
                    reinterpret_cast<int*>(arg3));

            if(*result >= 0) {
                reinterpret_cast<int*>(arg3)[0] = CTX->register_internal_fd(
                        reinterpret_cast<int*>(arg3)[0]);
                reinterpret_cast<int*>(arg3)[1] = CTX->register_internal_fd(
                        reinterpret_cast<int*>(arg3)[1]);
            }

            break;

        case SYS_pipe:
            *result = syscall_no_intercept(syscall_number,
                                           reinterpret_cast<int*>(arg0));

            if(*result >= 0) {
                reinterpret_cast<int*>(arg0)[0] = CTX->register_internal_fd(
                        reinterpret_cast<int*>(arg0)[0]);
                reinterpret_cast<int*>(arg0)[1] = CTX->register_internal_fd(
                        reinterpret_cast<int*>(arg0)[1]);
            }

            break;

        case SYS_pipe2:

            *result = syscall_no_intercept(syscall_number,
                                           reinterpret_cast<int*>(arg0),
                                           static_cast<int>(arg1));
            if(*result >= 0) {
                reinterpret_cast<int*>(arg0)[0] = CTX->register_internal_fd(
                        reinterpret_cast<int*>(arg0)[0]);
                reinterpret_cast<int*>(arg0)[1] = CTX->register_internal_fd(
                        reinterpret_cast<int*>(arg0)[1]);
            }

            break;

        case SYS_eventfd:

            *result = syscall_no_intercept(syscall_number,
                                           static_cast<int>(arg0));

            if(*result >= 0) {
                *result = CTX->register_internal_fd(*result);
            }
            break;

        case SYS_eventfd2:

            *result =
                    syscall_no_intercept(syscall_number, static_cast<int>(arg0),
                                         static_cast<int>(arg1));

            if(*result >= 0) {
                *result = CTX->register_internal_fd(*result);
            }
            break;

        case SYS_recvmsg: {
            *result =
                    syscall_no_intercept(syscall_number, static_cast<int>(arg0),
                                         reinterpret_cast<struct msghdr*>(arg1),
                                         static_cast<int>(arg2));

            // The recvmsg() syscall can receive file descriptors from another
            // process that the kernel automatically adds to the client's fds
            // as if dup2 had been called. Whenever that happens, we need to
            // make sure that we register these additional fds as internal, or
            // we could inadvertently overwrite them
            if(*result >= 0) {
                auto* hdr = reinterpret_cast<struct msghdr*>(arg1);
                struct cmsghdr* cmsg = CMSG_FIRSTHDR(hdr);

                for(; cmsg != NULL; cmsg = CMSG_NXTHDR(hdr, cmsg)) {
                    if(cmsg->cmsg_type == SCM_RIGHTS) {

                        size_t nfd = cmsg->cmsg_len > CMSG_LEN(0)
                                     ? (cmsg->cmsg_len - CMSG_LEN(0)) /
                                       sizeof(int)
                                     : 0;

                        int* fds = reinterpret_cast<int*>(CMSG_DATA(cmsg));

                        for(size_t i = 0; i < nfd; ++i) {
                            LOG(DEBUG, "recvmsg() provided extra fd {}",
                                fds[i]);

                            // ensure we update the fds in cmsg
                            // if they have been relocated
                            fds[i] = CTX->register_internal_fd(fds[i]);
                        }
                    }
                }
            }

            break;
        }

        case SYS_accept:
            *result = syscall_no_intercept(
                    syscall_number, static_cast<int>(arg0),
                    reinterpret_cast<struct sockaddr*>(arg1),
                    reinterpret_cast<int*>(arg2));

            if(*result >= 0) {
                *result = CTX->register_internal_fd(*result);
            }
            break;

        case SYS_fcntl:
            *result =
                    syscall_no_intercept(syscall_number, static_cast<int>(arg0),
                                         static_cast<int>(arg1), arg2);
            syserror = syscall_error_code(*result);

            if(syserror == 0) {


                if((static_cast<int>(arg1) == F_DUPFD ||
                    static_cast<int>(arg1) == F_DUPFD_CLOEXEC)) {
                    *result = CTX->register_internal_fd(*result);
                }
            }
            break;

        case SYS_close:
            *result = syscall_no_intercept(syscall_number,
                                           static_cast<int>(arg0));
            syserror = syscall_error_code(*result);

            if(syserror == 0) {
                CTX->unregister_internal_fd(arg0);
            }
            break;

        default:
            // ignore any other syscalls, i.e.: pass them on to the kernel
            // (syscalls forwarded to the kernel that return are logged in
            // hook_forwarded_syscall())
            //忽略任何其他的系统调用，例如:将它们传递给内核(转发到内核的系统调用返回时将记录在hook_forwarded_syscall()中)
            ::save_current_syscall_info(gaofs::syscall::from_internal_code |
                                        gaofs::syscall::to_kernel |
                                        gaofs::syscall::not_executed);
            return gaofs::syscall::forward_to_kernel;
    }

    LOG(SYSCALL,
        gaofs::syscall::from_internal_code | gaofs::syscall::to_hook |
        gaofs::syscall::executed,
        syscall_number, args, *result);

    return gaofs::syscall::hooked;
}

inline int hook(long syscall_number, long arg0, long arg1, long arg2, long arg3, long arg4,
                long arg5, long* result) {

    // gaofs_enable_logging   GAOFS_DEBUG_BUILD
#if defined(GAOFS_ENABLE_LOGGING) && defined(GAOFS_DEBUG_BUILD)
    const long args[gaofs::syscall::MAX_ARGS] = {arg0, arg1, arg2,
                                                arg3, arg4, arg5};
#endif

    LOG(SYSCALL,
        gaofs::syscall::from_external_code | gaofs::syscall::to_hook |
        gaofs::syscall::not_executed,
        syscall_number, args);

    switch(syscall_number) {

        case SYS_execve:
            *result = syscall_no_intercept(
                    syscall_number, reinterpret_cast<const char*>(arg0),
                    reinterpret_cast<const char* const*>(arg1),
                    reinterpret_cast<const char* const*>(arg2));
            break;

#ifdef SYS_execveat
        case SYS_execveat:
            *result = syscall_no_intercept(
                    syscall_number, arg0, reinterpret_cast<const char*>(arg1),
                    reinterpret_cast<const char* const*>(arg2),
                    reinterpret_cast<const char* const*>(arg3), arg4);
            break;
#endif

        case SYS_open:
            *result = gaofs::hook::hook_openat(
                    AT_FDCWD, reinterpret_cast<char*>(arg0),
                    static_cast<int>(arg1), static_cast<mode_t>(arg2));
            break;

        case SYS_creat:
            *result = gaofs::hook::hook_openat(
                    AT_FDCWD, reinterpret_cast<const char*>(arg0),
                    O_WRONLY | O_CREAT | O_TRUNC, static_cast<mode_t>(arg1));
            break;

        case SYS_openat:
            *result = gaofs::hook::hook_openat(
                    static_cast<int>(arg0), reinterpret_cast<const char*>(arg1),
                    static_cast<int>(arg2), static_cast<mode_t>(arg3));
            break;

        case SYS_close:
            *result = gaofs::hook::hook_close(static_cast<int>(arg0));
            break;

        case SYS_stat:
            *result =
                    gaofs::hook::hook_stat(reinterpret_cast<char*>(arg0),
                                          reinterpret_cast<struct stat*>(arg1));
            break;

#ifdef STATX_TYPE
        case SYS_statx:
            *result = gaofs::hook::hook_statx(
                    static_cast<int>(arg0), reinterpret_cast<char*>(arg1),
                    static_cast<int>(arg2), static_cast<unsigned int>(arg3),
                    reinterpret_cast<struct statx*>(arg4));
            break;
#endif

        case SYS_lstat:
            *result = gaofs::hook::hook_lstat(
                    reinterpret_cast<char*>(arg0),
                    reinterpret_cast<struct stat*>(arg1));
            break;

        case SYS_fstat:
            *result = gaofs::hook::hook_fstat(
                    static_cast<int>(arg0),
                    reinterpret_cast<struct stat*>(arg1));
            break;

        case SYS_newfstatat:
            *result = gaofs::hook::hook_fstatat(
                    static_cast<int>(arg0), reinterpret_cast<const char*>(arg1),
                    reinterpret_cast<struct stat*>(arg2),
                    static_cast<int>(arg3));
            break;

        case SYS_read:
            *result = gaofs::hook::hook_read(static_cast<unsigned int>(arg0),
                                            reinterpret_cast<void*>(arg1),
                                            static_cast<size_t>(arg2));
            break;

        case SYS_pread64:
            *result = gaofs::hook::hook_pread(static_cast<unsigned int>(arg0),
                                             reinterpret_cast<char*>(arg1),
                                             static_cast<size_t>(arg2),
                                             static_cast<loff_t>(arg3));
            break;

        case SYS_readv:
            *result = gaofs::hook::hook_readv(
                    static_cast<unsigned long>(arg0),
                    reinterpret_cast<const struct iovec*>(arg1),
                    static_cast<unsigned long>(arg2));
            break;

        case SYS_preadv:
            *result = gaofs::hook::hook_preadv(
                    static_cast<unsigned long>(arg0),
                    reinterpret_cast<const struct iovec*>(arg1),
                    static_cast<unsigned long>(arg2),
                    static_cast<unsigned long>(arg3),
                    static_cast<unsigned long>(arg4));
            break;

        case SYS_pwrite64:
            *result = gaofs::hook::hook_pwrite(
                    static_cast<unsigned int>(arg0),
                    reinterpret_cast<const char*>(arg1),
                    static_cast<size_t>(arg2), static_cast<loff_t>(arg3));
            break;
        case SYS_write:
            *result =
                    gaofs::hook::hook_write(static_cast<unsigned int>(arg0),
                                           reinterpret_cast<const char*>(arg1),
                                           static_cast<size_t>(arg2));
            break;

        case SYS_writev:
            *result = gaofs::hook::hook_writev(
                    static_cast<unsigned long>(arg0),
                    reinterpret_cast<const struct iovec*>(arg1),
                    static_cast<unsigned long>(arg2));
            break;

        case SYS_pwritev:
            *result = gaofs::hook::hook_pwritev(
                    static_cast<unsigned long>(arg0),
                    reinterpret_cast<const struct iovec*>(arg1),
                    static_cast<unsigned long>(arg2),
                    static_cast<unsigned long>(arg3),
                    static_cast<unsigned long>(arg4));
            break;

        case SYS_unlink:
            *result = gaofs::hook::hook_unlinkat(
                    AT_FDCWD, reinterpret_cast<const char*>(arg0), 0);
            break;

        case SYS_unlinkat:
            *result = gaofs::hook::hook_unlinkat(
                    static_cast<int>(arg0), reinterpret_cast<const char*>(arg1),
                    static_cast<int>(arg2));
            break;

        case SYS_rmdir:
            *result = gaofs::hook::hook_unlinkat(
                    AT_FDCWD, reinterpret_cast<const char*>(arg0),
                    AT_REMOVEDIR);
            break;

        case SYS_symlink:
            *result = gaofs::hook::hook_symlinkat(
                    reinterpret_cast<const char*>(arg0), AT_FDCWD,
                    reinterpret_cast<const char*>(arg1));
            break;

        case SYS_symlinkat:
            *result = gaofs::hook::hook_symlinkat(
                    reinterpret_cast<const char*>(arg0), static_cast<int>(arg1),
                    reinterpret_cast<const char*>(arg2));
            break;

        case SYS_access:
            *result =
                    gaofs::hook::hook_access(reinterpret_cast<const char*>(arg0),
                                            static_cast<int>(arg1));
            break;

        case SYS_faccessat:
            *result = gaofs::hook::hook_faccessat(
                    static_cast<int>(arg0), reinterpret_cast<const char*>(arg1),
                    static_cast<int>(arg2));
            break;
#ifdef SYS_faccessat2
            case SYS_faccessat2:
            *result = gaofs::hook::hook_faccessat2(
                    static_cast<int>(arg0), reinterpret_cast<const char*>(arg1),
                    static_cast<int>(arg2), static_cast<int>(arg3));
            break;
#endif
        case SYS_lseek:
            *result = gaofs::hook::hook_lseek(static_cast<unsigned int>(arg0),
                                             static_cast<off_t>(arg1),
                                             static_cast<unsigned int>(arg2));
            break;

        case SYS_truncate:
            *result = gaofs::hook::hook_truncate(
                    reinterpret_cast<const char*>(arg0),
                    static_cast<long>(arg1));
            break;

        case SYS_ftruncate:
            *result = gaofs::hook::hook_ftruncate(
                    static_cast<unsigned int>(arg0),
                    static_cast<unsigned long>(arg1));
            break;

        case SYS_dup:
            *result = gaofs::hook::hook_dup(static_cast<unsigned int>(arg0));
            break;

        case SYS_dup2:
            *result = gaofs::hook::hook_dup2(static_cast<unsigned int>(arg0),
                                            static_cast<unsigned int>(arg1));
            break;

        case SYS_dup3:
            *result = gaofs::hook::hook_dup3(static_cast<unsigned int>(arg0),
                                            static_cast<unsigned int>(arg1),
                                            static_cast<int>(arg2));
            break;

        case SYS_getdents:
            *result = gaofs::hook::hook_getdents(
                    static_cast<unsigned int>(arg0),
                    reinterpret_cast<struct linux_dirent*>(arg1),
                    static_cast<unsigned int>(arg2));
            break;

        case SYS_getdents64:
            *result = gaofs::hook::hook_getdents64(
                    static_cast<unsigned int>(arg0),
                    reinterpret_cast<struct linux_dirent64*>(arg1),
                    static_cast<unsigned int>(arg2));
            break;

        case SYS_mkdirat:
            *result = gaofs::hook::hook_mkdirat(
                    static_cast<unsigned int>(arg0),
                    reinterpret_cast<const char*>(arg1),
                    static_cast<mode_t>(arg2));
            break;

        case SYS_mkdir:
            *result = gaofs::hook::hook_mkdirat(
                    AT_FDCWD, reinterpret_cast<const char*>(arg0),
                    static_cast<mode_t>(arg1));
            break;

        case SYS_chmod:
            *result = gaofs::hook::hook_fchmodat(AT_FDCWD,
                                                reinterpret_cast<char*>(arg0),
                                                static_cast<mode_t>(arg1));
            break;

        case SYS_fchmod:
            *result = gaofs::hook::hook_fchmod(static_cast<unsigned int>(arg0),
                                              static_cast<mode_t>(arg1));
            break;

        case SYS_fchmodat:
            *result = gaofs::hook::hook_fchmodat(static_cast<unsigned int>(arg0),
                                                reinterpret_cast<char*>(arg1),
                                                static_cast<mode_t>(arg2));
            break;

        case SYS_chdir:
            *result =
                    gaofs::hook::hook_chdir(reinterpret_cast<const char*>(arg0));
            break;

        case SYS_fchdir:
            *result = gaofs::hook::hook_fchdir(static_cast<unsigned int>(arg0));
            break;

        case SYS_getcwd:
            *result = gaofs::hook::hook_getcwd(reinterpret_cast<char*>(arg0),
                                              static_cast<unsigned long>(arg1));
            break;

        case SYS_readlink:
            *result = gaofs::hook::hook_readlinkat(
                    AT_FDCWD, reinterpret_cast<const char*>(arg0),
                    reinterpret_cast<char*>(arg1), static_cast<int>(arg2));
            break;

        case SYS_readlinkat:
            *result = gaofs::hook::hook_readlinkat(
                    static_cast<int>(arg0), reinterpret_cast<const char*>(arg1),
                    reinterpret_cast<char*>(arg2), static_cast<int>(arg3));
            break;

        case SYS_fcntl:
            *result = gaofs::hook::hook_fcntl(static_cast<unsigned int>(arg0),
                                             static_cast<unsigned int>(arg1),
                                             static_cast<unsigned long>(arg2));
            break;

        case SYS_rename:
            *result = gaofs::hook::hook_renameat(
                    AT_FDCWD, reinterpret_cast<const char*>(arg0), AT_FDCWD,
                    reinterpret_cast<const char*>(arg1), 0);
            break;

        case SYS_renameat:
            *result = gaofs::hook::hook_renameat(
                    static_cast<int>(arg0), reinterpret_cast<const char*>(arg1),
                    static_cast<int>(arg2), reinterpret_cast<const char*>(arg3),
                    0);
            break;

        case SYS_renameat2:
            *result = gaofs::hook::hook_renameat(
                    static_cast<int>(arg0), reinterpret_cast<const char*>(arg1),
                    static_cast<int>(arg2), reinterpret_cast<const char*>(arg3),
                    static_cast<unsigned int>(arg4));
            break;

        case SYS_fstatfs:
            *result = gaofs::hook::hook_fstatfs(
                    static_cast<unsigned int>(arg0),
                    reinterpret_cast<struct statfs*>(arg1));
            break;

        case SYS_statfs:
            *result = gaofs::hook::hook_statfs(
                    reinterpret_cast<const char*>(arg0),
                    reinterpret_cast<struct statfs*>(arg1));
            break;

        case SYS_fsync:
            *result = gaofs::hook::hook_fsync(static_cast<unsigned int>(arg0));
            break;

        case SYS_getxattr:
            *result = gaofs::hook::hook_getxattr(
                    reinterpret_cast<const char*>(arg0),
                    reinterpret_cast<const char*>(arg1),
                    reinterpret_cast<void*>(arg2), static_cast<size_t>(arg4));
            break;

        default:
            // ignore any other syscalls, i.e.: pass them on to the kernel
            // (syscalls forwarded to the kernel that return are logged in
            // hook_forwarded_syscall())
            ::save_current_syscall_info(gaofs::syscall::from_external_code |
                                        gaofs::syscall::to_kernel |
                                        gaofs::syscall::not_executed);
            return gaofs::syscall::forward_to_kernel;
    }

    LOG(SYSCALL,
        gaofs::syscall::from_external_code | gaofs::syscall::to_hook |
        gaofs::syscall::executed,
        syscall_number, args, *result);

    return gaofs::syscall::hooked;
}

#ifdef SYS_socketcall
/* Wraps socketcall in powerpc9, we only change syscalls that need special
 * treatment */
//在powerpc9中包装套接字调用，我们只更改需要特殊处理的系统调用
long socketcall_wrapper(long syscall_number, long& arg0, long& arg1, long& arg2,
                   long& arg3, long& arg4, long& arg5) {

    if(syscall_number == SYS_socketcall) {
        switch(static_cast<int>(arg0)) {
            case 1:
                syscall_number = SYS_socket;
                break;
            case 4:
                syscall_number = SYS_listen;
                break;
            case 5:
                syscall_number = SYS_accept;
                break;
            case 8:
                syscall_number = SYS_socketpair;
                break;
            case 13:
                syscall_number = SYS_shutdown;
                break;
            case 17:
                syscall_number = SYS_recvmsg;
                break;
            case 18:
                syscall_number = SYS_accept4;
                break;
            case 19:
                syscall_number = SYS_recvmmsg;
                break;
            default:
                break;
        }

        long int* parameters = (long int*) arg1;
        arg0 = static_cast<long>(*parameters);
        parameters++;
        arg1 = static_cast<long>(*parameters);
        parameters++;
        arg2 = static_cast<long>(*parameters);
        parameters++;
        arg3 = static_cast<long>(*parameters);
        parameters++;
        arg4 = static_cast<long>(*parameters);
    }
    return syscall_number;
}
#endif

//
void hook_forwarded_syscall(long syscall_number, long arg0, long arg1, long arg2,
                       long arg3, long arg4, long arg5, long result) {

    if(::get_current_syscall_info() == gaofs::syscall::no_info) {
        return;
    }

#if defined(GAOFS_ENABLE_LOGGING) && defined(GAOFS_DEBUG_BUILD)
        const long args[gaofs::syscall::MAX_ARGS] = {arg0, arg1, arg2,
                                            arg3, arg4, arg5};
#endif

    LOG(SYSCALL, ::get_current_syscall_info() | gaofs::syscall::executed,
        syscall_number, args, result);

    ::reset_current_syscall_info();
}

void hook_clone_at_child(unsigned long flags, void* child_stack, int* ptid,
                    int* ctid, long newtls) {

#if defined(GAOFS_ENABLE_LOGGING) && defined(GAOFS_DEBUG_BUILD)
    const long args[gaofs::syscall::MAX_ARGS] = {
        static_cast<long>(flags),     reinterpret_cast<long>(child_stack),
        reinterpret_cast<long>(ptid), reinterpret_cast<long>(ctid),
        static_cast<long>(newtls),    0};
#endif

    reentrance_guard_flag = true;

    LOG(SYSCALL, ::get_current_syscall_info() | gaofs::syscall::executed,
        SYS_clone, args, 0);

    reentrance_guard_flag = false;
}

//
void hook_clone_at_parent(unsigned long flags, void* child_stack, int* ptid,
                     int* ctid, long newtls, long returned_pid) {

#if defined(GAOFS_ENABLE_LOGGING) && defined(GAOFS_DEBUG_BUILD)
    const long args[gaofs::syscall::MAX_ARGS] = {
        static_cast<long>(flags),     reinterpret_cast<long>(child_stack),
        reinterpret_cast<long>(ptid), reinterpret_cast<long>(ctid),
        static_cast<long>(newtls),    0};
#endif

    reentrance_guard_flag = true;

    LOG(SYSCALL, ::get_current_syscall_info() | gaofs::syscall::executed,
        SYS_clone, args, returned_pid);

    reentrance_guard_flag = false;
}


} // namespace

namespace gaofs::preload {

// lxl 对internal_hook的封装
int internal_hook_guard_wrapper(long syscall_number, long arg0, long arg1,
    long arg2, long arg3, long arg4, long arg5,
    long* syscall_return_value) {
    assert(CTX->interception_enabled());

#ifdef SYS_socketcall
    if(syscall_number == SYS_socketcall)
syscall_number = socketcall_wrapper(syscall_number, arg0, arg1, arg2,
                                    arg3, arg4, arg5);
#endif

    if(reentrance_guard_flag) {
        ::save_current_syscall_info(gaofs::syscall::from_internal_code |
                                    gaofs::syscall::to_kernel |
                                    gaofs::syscall::not_executed);
        return gaofs::syscall::forward_to_kernel;
    }

    int was_hooked = 0;

    reentrance_guard_flag = true;
    int oerrno = errno;
    was_hooked = hook_internal(syscall_number, arg0, arg1, arg2, arg3, arg4,
                               arg5, syscall_return_value);
    errno = oerrno;
    reentrance_guard_flag = false;

    return was_hooked;
}


/*

* reentrance_guard_flag标志允许库区分自己的系统调用的钩子。例如，在处理open()1系统调用时，
* libgaofs_intercept可能调用fopen()， fopen()在内部使用open()2系统调用。这个内部使用的open()2系统调用再次被转发到libgaofs_intercept，
* 但是使用这个标志，我们可以注意到重新输入自身的情况。
* // 按这个意思就是open1是远程调用，open2是本地的，通过reentrance_guard_flag区分，就是用reentrance_guard_flag区分internal hook和 hook？

* 这种方法仍然包含一个非常重要的错误，因为在信号处理程序内部调用libgaofs_intercept可能很容易将模拟fd转发到内核。
*/
int hook_guard_wrapper(long syscall_number, long arg0, long arg1, long arg2,
    long arg3, long arg4, long arg5,
    long* syscall_return_value) {

    assert(CTX->interception_enabled());

#ifdef SYS_socketcall
    if(syscall_number == SYS_socketcall)
syscall_number = socketcall_wrapper(syscall_number, arg0, arg1, arg2,
                                    arg3, arg4, arg5);
#endif

    int was_hooked = 0;

    if(reentrance_guard_flag) {
        int oerrno = errno;
        was_hooked = hook_internal(syscall_number, arg0, arg1, arg2, arg3, arg4,
                                   arg5, syscall_return_value);
        errno = oerrno;
        return was_hooked;
    }

    reentrance_guard_flag = true;
    int oerrno = errno;
    was_hooked = ::hook(syscall_number, arg0, arg1, arg2, arg3, arg4, arg5,
                        syscall_return_value);
    errno = oerrno;
    reentrance_guard_flag = false;

    return was_hooked;
}

void start_self_interception() {

    LOG(DEBUG, "Enabling syscall interception for self");

    intercept_hook_point = internal_hook_guard_wrapper;
    intercept_hook_point_post_kernel = hook_forwarded_syscall;
    intercept_hook_point_clone_child = hook_clone_at_child;
    intercept_hook_point_clone_parent = hook_clone_at_parent;
}

void start_interception() {

    assert(CTX->interception_enabled());

    LOG(DEBUG, "Enabling syscall interception for client process");

    // Set up the callback function pointer
    intercept_hook_point = hook_guard_wrapper;
    intercept_hook_point_post_kernel = hook_forwarded_syscall;
    intercept_hook_point_clone_child = hook_clone_at_child;
    intercept_hook_point_clone_parent = hook_clone_at_parent;
}

void stop_interception() {
    assert(CTX->interception_enabled());

    LOG(DEBUG, "Disabling syscall interception for client process");

    // Reset callback function pointer
    intercept_hook_point = nullptr;
    intercept_hook_point_post_kernel = nullptr;
    intercept_hook_point_clone_child = nullptr;
    intercept_hook_point_clone_parent = nullptr;
}

} // namespace gaofs::preload




