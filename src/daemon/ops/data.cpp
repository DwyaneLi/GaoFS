//
// Created by DELL on 2023/2/19.
//

#include <daemon/ops/data.hpp>
#include <daemon/ops/metadentry.hpp>
#include <daemon/backend/data/chunk_storage.hpp>
#include <common/arithmetic/arithmetic.hpp>
#include <daemon/backend/metadata/db.hpp>
#include <algorithm>
extern "C" {
#include <mercury_types.h>
}

using namespace std;

namespace gaofs::data {
/* ------------------------------------------------------------------------
* -------------------------- TRUNCATE ------------------------------------
* ------------------------------------------------------------------------*/

// 构造函数
ChunkTruncateOperation::ChunkTruncateOperation(const string &path) : ChunkOperation(path,1) {};

// 清除参数
void ChunkTruncateOperation::clear_task_args() {
    task_arg_ = {};
}

// 由Argobots微线程独家使用。参数args有以下字段:
// const string* path;
// size_t size;
// ABT_eventual* eventual;
// 该函数由IO池驱动。因此，有一个每个守护进程允许的最大并发操作数。
void ChunkTruncateOperation::truncate_abt(void *_arg) {
    // 要使用pow2
    using namespace gaofs::utils::arithmetic;
    assert(_arg);
    auto* arg = static_cast<struct chunk_truncate_args*>(_arg);
    const string& path = *(arg->path);
    const size_t size = arg->size;
    const uint64_t host_id = arg->host_id;
    const uint64_t host_size = arg->host_size;
    int err_response = 0;
    try {
        // 算出要截断的起始的chunk_id
        auto chunk_id_start = block_index(size, gaofs::config::rpc::chunksize);
        // 查看超出块起始多少，如果在这个数据块的中间，则不能删除这个块
        auto left_pad = block_overrun(size, gaofs::config::rpc::chunksize);
        // 如果第0块在当前结点上
        if(RPC_DATA->distributor()->locate_data(path, chunk_id_start, host_size) == host_id) {
            // TODO 如果是firstchunk, 目前的做法是就算leftpad为0，也不删掉first_chunk
            if(chunk_id_start == 0) {
                auto firstChunk = gaofs::metadata::get_first_chunk(path);
                firstChunk.size(left_pad);
                firstChunk.content(firstChunk.content().substr(0, left_pad));
                gaofs::metadata::update_first_chunk(path, firstChunk);
                chunk_id_start++;
            } else {
                if(left_pad != 0) {
                    // 把单个块文件给截断
                    GAOFS_DATA->storage()->truncate_chunk_file(path, chunk_id_start, left_pad);

                    // ++是因为后面的chunk全都要被删除
                    chunk_id_start++;
                }
            }
        }
        /*
        if(left_pad != 0) {
            // 把单个块文件给截断
            GAOFS_DATA->storage()->truncate_chunk_file(path, chunk_id_start, left_pad);

            // ++是因为后面的chunk全都要被删除
            chunk_id_start++;
        }
         */
        // 删除后面的所有chunk
        GAOFS_DATA->storage()->trim_chunk_space(path, chunk_id_start);
    } catch(const ChunkStorageException& err) {
        GAOFS_DATA->spdlogger()->error(
                "{}() {}",
                __func__, err.what());
        err_response = err.code().value();
    } catch(const gaofs::db_exception::NotFoundException& err) {
        GAOFS_DATA->spdlogger()->error(
                "{}() First chunk entry not found: '{}'",
                __func__, path);
        err_response = ENOENT;
    } catch(const gaofs::db_exception::DBException& err) {
        GAOFS_DATA->spdlogger()->error(
                "{}(): path '{}' message '{}'",
                __func__, path, err.what());
        err_response = EIO;
    } catch(const exception& err) {
        GAOFS_DATA->spdlogger()->error(
                "{}() Unexpected error truncating file '{}' to length '{}'",
                __func__, path, size);
        err_response = EIO;
    }

    // 把最后的结构放到回调中
    ABT_eventual_set(arg->eventual, &err_response, sizeof(err_response));
}

// 为截断操作启动微线程
void ChunkTruncateOperation::truncate(size_t size, uint64_t host_id, uint64_t host_size) {
    // 第一个回调一定要是空的，因为一个操作只允许有一个
    assert(!task_eventuals_[0]);
    GAOFS_DATA->spdlogger()->trace(
            "ChunkTruncateOperation::{}() path '{}' size '{}'",
            __func__, path_, size);

    auto abt_err = ABT_eventual_create(sizeof(int), &task_eventuals_[0]);
    if(abt_err != ABT_SUCCESS) {
        // 创建回调失败
        auto err_str = fmt::format(
                "ChunkTruncateOperation::{}() Failed to create ABT eventual with abt_err '{}'",
                __func__, abt_err);
        throw ChunkMetaOpException(err_str);
    }

    // 如果创建成功，就构建参数
    auto& task_arg = task_arg_;
    task_arg.size = size;
    task_arg.path = &path_;
    task_arg.host_id = host_id;
    task_arg.host_size = host_size;
    task_arg_.eventual = task_eventuals_[0];
    // 启动微线程并且等着接受回调
    abt_err = ABT_task_create(RPC_DATA->io_pool(), truncate_abt, &task_arg_, &abt_tasks_[0]);

    if(abt_err != ABT_SUCCESS) {
        auto err_str = fmt::format(
                "ChunkTruncateOperation::{}() Failed to create ABT task with abt_err '{}'",
                __func__, abt_err);
        throw ChunkMetaOpException(err_str);
    }
}

int ChunkTruncateOperation::wait_for_task() {
    GAOFS_DATA->spdlogger()->trace(
            "ChunkTruncateOperation::{}() enter: path '{}'",
            __func__, path_);
    int trunc_err = 0;
    int* task_err = nullptr;
    auto abt_err = ABT_eventual_wait(task_eventuals_[0], (void**) &task_err);

    // 如果wait出错了
    if(abt_err != ABT_SUCCESS) {
        GAOFS_DATA->spdlogger()->error(
                "ChunkTruncateOperation::{}() Error when waiting on ABT eventual",
                __func__);
        // 释放回调
        ABT_eventual_free(&task_eventuals_[0]);
        return EIO;
    }
    assert(task_err != nullptr);
    if(*task_err != 0) {
        trunc_err = *task_err;
    }
    ABT_eventual_free(&task_eventuals_[0]);
    return trunc_err;
}

/* ------------------------------------------------------------------------
 * ----------------------------- WRITE ------------------------------------
 * ------------------------------------------------------------------------*/

// 构造函数，因为是一对多，n就是所对chunk块的数量
ChunkWriteOperation::ChunkWriteOperation(const string &path, size_t n) : ChunkOperation(path, n) {
    task_args_.resize(n);
}

// 清空参数
void ChunkWriteOperation::clear_task_args() {
    task_args_.clear();
}

// 由Argobots微线程独家使用。参数args有以下字段:
//    * const string* path;
//    const char* buf;
//    const gkfs::rpc::chnk_id_t* chnk_id;
//    size_t size;
//    off64_t off;
//    ABT_eventual eventual;
// 该函数由IO池驱动。因此，有一个每个守护进程允许的最大并发操作数。
void ChunkWriteOperation::write_file_abt(void *_arg) {
    assert(_arg);
    auto* arg = static_cast<chunk_write_args*>(_arg);
    const string& path = *(arg->path);
    // 记录写的数量
    ssize_t wrote{0};
    try {
        // 如果是first_chunk,则进行相关处理
        if(arg->chnk_id == 0) {
            assert((arg->off+arg->size) <= gaofs::config::rpc::chunksize);
            auto firstChunk = gaofs::metadata::get_first_chunk(path);;
            auto content = firstChunk.content();
            // 分情况处理，如果offset + size 比之前的first chunk的内容， content得扩容
            if(arg->off + arg->size > content.size()) {
                content.resize(arg->size + arg->off);
            }

            std::copy_n(arg->buf, arg->size, content.begin() + arg->off);
            firstChunk.content(content);
            firstChunk.size(content.size());
            gaofs::metadata::update_first_chunk(path, firstChunk);
            wrote = arg->size;
        } else {
            wrote = GAOFS_DATA->storage()->write_chunk(path, arg->chnk_id, arg->buf,
                                                       arg->size, arg->off);
        }
        /*
        wrote = GAOFS_DATA->storage()->write_chunk(path, arg->chnk_id, arg->buf,
                                                   arg->size, arg->off);
        */
    } catch(const ChunkStorageException& err) {
        GAOFS_DATA->spdlogger()->error(
                "{}() {}",__func__, err.what());
        // 用负数来表示出错了，因为wrote可能是任何正数
        wrote = -(err.code().value());
    } catch(const gaofs::db_exception::NotFoundException& err) {
        GAOFS_DATA->spdlogger()->error(
                "{}() First chunk entry not found: '{}'",
                __func__, path);
        wrote = -ENOENT;
    } catch(const gaofs::db_exception::DBException& err) {
        GAOFS_DATA->spdlogger()->error(
                "{}(): path '{}' message '{}'",
                __func__, path, err.what());
        wrote = -EIO;
    } catch(const exception& err) {
        GAOFS_DATA->spdlogger()->error(
                "{}() Unexpected error writing chunk {} of file {}",
                __func__, arg->chnk_id, path);
        wrote = -EIO;
    }
    ABT_eventual_set(arg->eventual, &wrote, sizeof(wrote));
}

// 用于启动微线程并且调动write_file_abt来执行具体操作
void ChunkWriteOperation::write_nonblock(size_t idx, uint64_t chunk_id, const char *bulk_buf_ptr, size_t size,
                                         off64_t offset) {
    assert(idx < task_args_.size());
    GAOFS_DATA->spdlogger()->trace(
            "ChunkWriteOperation::{}() enter: idx '{}' path '{}' size '{}' offset '{}'",
            __func__, idx, path_, size, offset);
    // 创建一个回调
    auto abt_err = ABT_eventual_create(sizeof(ssize_t), &task_eventuals_[idx]);
    if(abt_err != ABT_SUCCESS) {
        auto err_str = fmt::format(
                "ChunkWriteOperation::{}() Failed to create ABT eventual with abt_err '{}'",
                __func__, abt_err);
        throw ChunkWriteOpException(err_str);
    }

    // 构建对应的参数
    auto& task_arg = task_args_[idx];
    task_arg.size = size;
    task_arg.chnk_id = chunk_id;
    task_arg.off = offset;
    task_arg.buf = bulk_buf_ptr;
    task_arg.path = &path_;
    task_arg.eventual = task_eventuals_[idx];

    // 启动微线程并且等着接受回调
    abt_err = ABT_task_create(RPC_DATA->io_pool(), write_file_abt, &task_args_[idx], &abt_tasks_[idx]);
    if(abt_err != ABT_SUCCESS) {
        auto err_str = fmt::format(
                "ChunkWriteOperation::{}() Failed to create ABT task with abt_err '{}'",
                __func__, abt_err);
        throw ChunkWriteOpException(err_str);
    }
}

// 等待任务执行
std::pair<int, size_t> ChunkWriteOperation::wait_for_tasks() {
    GAOFS_DATA->spdlogger()->trace("ChunkWriteOperation::{}() enter: path '{}'",
                                   __func__, path_);
    // 计算总写入的数量
    size_t total_written = 0;
    // io_err作为返回值看是否出错
    int io_err = 0;
    for(auto& e : task_eventuals_) {
        ssize_t* task_size = nullptr;
        auto abt_err = ABT_eventual_wait(e, (void**)&task_size);
        if(abt_err != ABT_SUCCESS) {
            GAOFS_DATA->spdlogger()->error(
                    "ChunkWriteOperation::{}() Error when waiting on ABT eventual",
                    __func__);
            io_err = EIO;
            ABT_eventual_free(&e);
            continue;
        }

        // 如果有一个块出问题了，后面的直接就释放了，就不管了
        if(io_err != 0) {
            ABT_eventual_free(&e);
        }

        assert(task_size != nullptr);
        if(*task_size < 0) {
            io_err = -(*task_size);
        } else {
            total_written += *task_size;
        }
        ABT_eventual_free(&e);
    }

    // 在发生错误时，写入大小设置为0，因为数据将被损坏
    if(io_err != 0) {
        total_written = 0;
    }
    return make_pair(io_err, total_written);
}

// 下面的是read的，其实read和write差不多，主要是回调那里有点区别
/* ------------------------------------------------------------------------
 * -------------------------- READ ----------------------------------------
 * ------------------------------------------------------------------------*/

// 构造函数
ChunkReadOperation::ChunkReadOperation(const string &path, size_t n) : ChunkOperation(path, n) {
    task_args_.resize(n);
}

// 清空参数
void ChunkReadOperation::clear_task_args() {
    task_args_.clear();
}

// 由Argobots微线程独家使用。参数args有以下字段:
//    * const string* path;
//    char* buf;
//    const gkfs::rpc::chnk_id_t* chnk_id;
//    size_t size;
//   off64_t off;
//    ABT_eventual* eventual;
// 该函数由IO池驱动。因此，有一个每个守护进程允许的最大并发操作数。
void ChunkReadOperation::read_file_abt(void *_arg) {
    assert(_arg);
    auto* arg = static_cast<chunk_read_args*>(_arg);
    const string& path = *(arg->path);
    ssize_t read = 0;

    try {
        // 对first chunk的处理
        if(arg->chnk_id == 0) {
            auto content = gaofs::metadata::get_content_first_chunk(path);
            if(arg->size + arg->off >= content.size()) {
                if(arg->off >= content.size()) {
                    read = 0;
                } else {
                    read = content.size() - arg->off;
                    std::copy(content.begin()+arg->off, content.end(), arg->buf);
                }
            } else {
                read = arg->size;
                std::copy_n(content.begin()+arg->off, arg->size, arg->buf);
            }
        } else {
            read = GAOFS_DATA->storage()->read_chunk(path, arg->chnk_id, arg->buf,
                                                     arg->size, arg->off);
        }
        /*
        read = GAOFS_DATA->storage()->read_chunk(path, arg->chnk_id, arg->buf,
                                                 arg->size, arg->off);
        */
    } catch(const ChunkStorageException& err) {
        GAOFS_DATA->spdlogger()->error("{}() {}", __func__, err.what());
        read = -(err.code().value());
    } catch(const gaofs::db_exception::NotFoundException& err) {
        GAOFS_DATA->spdlogger()->error(
                "{}() First chunk entry not found: '{}'",
                __func__, path);
        read = -ENOENT;
    } catch(const gaofs::db_exception::DBException& err) {
        GAOFS_DATA->spdlogger()->error(
                "{}(): path '{}' message '{}'",
                __func__, path, err.what());
        read = -EIO;
    } catch(const exception& err) {
        GAOFS_DATA->spdlogger()->error(
                "{}() Unexpected error reading chunk {} of file {}",
                __func__, arg->chnk_id, path);
        read = EIO;
    }
    ABT_eventual_set(arg->eventual, &read, sizeof(read));
}

void ChunkReadOperation::read_nonblock(size_t idx, uint64_t chunk_id, char *bulk_buf_ptr, size_t size,
                                       off64_t offset) {
    assert(idx < task_args_.size());
    GAOFS_DATA->spdlogger()->trace(
            "ChunkReadOperation::{}() enter: idx '{}' path '{}' size '{}' offset '{}'",
            __func__, idx, path_, size, offset);

    // 创建回调
    auto abt_err = ABT_eventual_create(sizeof(ssize_t), &task_eventuals_[idx]);
    if(abt_err != ABT_SUCCESS) {
        auto err_str = fmt::format(
                "ChunkReadOperation::{}() Failed to create ABT eventual with abt_err '{}'",
                __func__, abt_err);
        throw ChunkReadOpException(err_str);
    }

    // 构造参数
    auto& task_arg = task_args_[idx];
    task_arg.path = &path_;
    task_arg.size = size;
    task_arg.off = offset;
    task_arg.chnk_id = chunk_id;
    task_arg.buf = bulk_buf_ptr;
    task_arg.eventual = task_eventuals_[idx];

    // 和rpc_data有关  启动微线程并且等着接受回调
    abt_err = ABT_task_create(RPC_DATA->io_pool(), read_file_abt, &task_args_[idx], &abt_tasks_[idx]);
    if(abt_err != ABT_SUCCESS) {
        auto err_str = fmt::format(
                "ChunkReadOperation::{}() Failed to create ABT task with abt_err '{}'",
                __func__, abt_err);
        throw ChunkReadOpException(err_str);
    }
}

// 这个函数与写不同的是，要完成将buf推到源的任务
std::pair<int, size_t> ChunkReadOperation::wait_for_tasks_and_push_back(const bulk_args &args) {
    GAOFS_DATA->spdlogger()->trace("ChunkReadOperation::{}() enter: path '{}'",
                                   __func__, path_);
    assert(args.chunk_ids->size() == task_args_.size());
    size_t total_read = 0;
    int io_err = 0;

    // 收集最终的所有信息。一旦遇到错误，bulk_transfers将不再执行，因为数据将被损坏。
    // 循环将继续，直到所有的eventuals被清理和释放。
    for(uint64_t idx = 0; idx < task_args_.size(); idx++) {
        ssize_t* task_size = nullptr;
        auto abt_err = ABT_eventual_wait(task_eventuals_[idx], (void**)&task_size);
        if(abt_err != ABT_SUCCESS) {
            GAOFS_DATA->spdlogger()->error(
                    "ChunkReadOperation::{}() Error when waiting on ABT eventual",
                    __func__);
            io_err = EIO;
            ABT_eventual_free(&task_eventuals_[idx]);
            continue;
        }

        // 如果之前的发生了错误，直接清空回调
        if(io_err != 0) {
            ABT_eventual_free(&task_eventuals_[idx]);
            continue;
        }
        assert(task_size != nullptr);
        if(*task_size < 0) {
            // *task_size 小于零一般是认为出问题了，但可能有特殊情况就是task_size = -ENOENT,可能是块文件不存在
            // 这种情况下不认为出错了
            if((*task_size) == -ENOENT) {
                ABT_eventual_free(&task_eventuals_[idx]);
                continue;
            }
            io_err = -(*task_size);
        } else if(*task_size == 0) {
            // 读取大小为0不是一个错误，可能发生的原因是读取文件末尾
            ABT_eventual_free(&task_eventuals_[idx]);
            continue;
        } else {
            // 成功读取到数据，要把读到的数据推到客户端，并且得释放回调
            GAOFS_DATA->spdlogger()->trace(
                    "ChunkReadOperation::{}() BULK_TRANSFER_PUSH file '{}' chnkid '{}' origin offset '{}' local offset '{}' transfersize '{}'",
                    __func__, path_, args.chunk_ids->at(idx), args.origin_offsets->at(idx) ,args.local_offsets->at(idx), *task_size);
            assert(task_args_[idx].chnk_id == args.chunk_ids->at(idx));
            auto margo_err = margo_bulk_transfer(args.mid, HG_BULK_PUSH, args.origin_addr,
                                                 args.origin_bulk_handle, args.origin_offsets->at(idx),
                                                 args.local_bulk_handle, args.local_offsets->at(idx),
                                                 *task_size);
            if(margo_err != HG_SUCCESS) {
                GAOFS_DATA->spdlogger()->error(
                        "ChunkReadOperation::{}() Failed to margo_bulk_transfer with margo err: '{}'",
                        __func__, margo_err);
                io_err = EBUSY;
                continue;
            }
            total_read += *task_size;
        }
        ABT_eventual_free(&task_eventuals_[idx]);
    }

    //在发生错误时，写入大小设置为0，因为数据将被损坏
    if(io_err != 0 ) {
        total_read = 0;
    }
    return make_pair(io_err, total_read);
}

} // namespace gaofs::data
