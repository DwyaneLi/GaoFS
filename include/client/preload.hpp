//
// Created by DELL on 2023/3/20.
//

#ifndef GAOFS_PRELOAD_HPP
#define GAOFS_PRELOAD_HPP

#include <client/preload_context.hpp>

#define EUNKNOWN (-1)

// preloadcontext实例
#define  CTX gaofs::preload::PreloadContext::getInstance()

namespace gaofs::preload {

void init_ld_env_if_needed();

} // namespace gaofs::preload

//constructor参数让系统执行main()函数之前调用函数(被__attribute__((constructor))修饰的函数).
//同理, destructor让系统在main()函数退出或者调用了exit()之后,调用我们的函数.
void init_preload() __attribute__((constructor));

void destory_preload() __attribute__((destructor));

#endif //GAOFS_PRELOAD_HPP
