//
// Created by DELL on 2023/3/29.
//

#ifndef GAOFS_INTERCEPT_HPP
#define GAOFS_INTERCEPT_HPP

namespace gaofs::preload {

// 内部系统调用的封装
int internal_hook_guard_wrapper(long syscall_number, long arg0, long arg1,
                            long arg2, long arg3, long arg4, long arg5,
                            long* syscall_return_value);
// gaofs系统调用的封装
int hook_guard_wrapper(long syscall_number, long arg0, long arg1, long arg2,
                   long arg3, long arg4, long arg5, long* syscall_return_value);

void start_self_interception();

void start_interception();

void stop_interception();

} // namespace gaofs::preload

#endif //GAOFS_INTERCEPT_HPP
