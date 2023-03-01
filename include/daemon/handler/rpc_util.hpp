//
// Created by DELL on 2023/2/28.
//

#ifndef GAOFS_RPC_UTIL_HPP
#define GAOFS_RPC_UTIL_HPP

extern "C" {
#include <mercury_types.h>
#include <margo.h>
};

namespace gaofs::rpc{

// 释放所有分配的rpc资源
// handle: 指向Mercury RPC handle的指针
// bulk_handle: 指向Mercury bulk handle的指针
template <typename InputType, typename  OutputType>
inline hg_return_t cleanup(hg_handle_t* handle, InputType* input, OutputType* output, hg_bulk_t* bulk_handle) {
    auto ret = HG_SUCCESS;
    // 分别释放上述所有指针指向的资源
    if(bulk_handle) {
        ret = margo_bulk_free(*bulk_handle);
        if(ret != HG_SUCCESS) {
            return ret;
        }
    }

    if(input && handle) {
        ret = margo_free_input(*handle, input);
        if(ret != HG_SUCCESS) {
            return ret;
        }
    }

    if(output && handle) {
        ret = margo_free_output(*handle, output);
        if(ret != HG_SUCCESS) {
            return ret;
        }
    }

    if(handle) {
        ret = margo_destroy(*handle);
        if(ret != HG_SUCCESS) {
            return ret;
        }
    }
    return ret;
}

// server响应客户端
template <typename OutputType>
inline hg_return_t respond(hg_handle_t* handle, OutputType* output) {
    auto ret = HG_SUCCESS;
    if(output && handle) {
        ret = margo_respond(*handle, output);
        if(ret != HG_SUCCESS) {
            return ret;
        }
    }
    return ret;
}

// lxl 把上面两个组合，先响应，再释放，下面两个同理
// 但是不释放output
// Mercury在响应客户机后释放输出结构本身,如果自己显式释放可能会出现错误
template <typename InputType, typename  OutputType>
inline hg_return_t cleanup_respond(hg_handle_t* handle, InputType* input, OutputType* output, hg_bulk_t* bulk_handle) {
    auto ret = respond(handle, output);
    if(ret != HG_SUCCESS) {
        return ret;
    }
    return cleanup(handle, input, static_cast<OutputType*>(nullptr), bulk_handle);
}

// 没有bulk的释放
template <typename InputType, typename  OutputType>
inline hg_return_t cleanup_respond(hg_handle_t* handle, InputType* input, OutputType* output) {
    return cleanup_respond(handle, input, output, nullptr);
}

// 没有input， 没有bulk的释放
template <typename  OutputType>
inline hg_return_t cleanup_respond(hg_handle_t* handle, OutputType* output) {
    auto ret = respond(handle, output);
    if(ret != HG_SUCCESS) {
        return ret;
    }
    if(handle) {
        ret = margo_destroy(*handle);
        if(ret != HG_SUCCESS) {
            return ret;
        }
    }
    return ret;
}

} // namespace gaofs::rpc

#endif //GAOFS_RPC_UTIL_HPP
