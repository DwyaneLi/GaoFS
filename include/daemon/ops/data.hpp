//
// Created by DELL on 2023/2/19.
//

#ifndef GAOFS_DATA_HPP
#define GAOFS_DATA_HPP

#include <daemon/daemon.hpp>
#include <common/common_defs.hpp>

#include <string>
#include <vector>

extern "C" {
#include <abt.h>
#include <margo.h>
};

namespace gaofs::data {

// chunk操作时会出现的异常的封装类
// 父类
class ChunkOpException : public std::runtime_error {
public:
    explicit ChunkOpException(std::string s) : std::runtime_error(s) {};
};

class ChunkWriteOpException : public ChunkOpException {
public:
    explicit ChunkWriteOpException(std::string s) : ChunkOpException(s) {};
};

class ChunkReadOpException : public ChunkOpException {
public:
    explicit ChunkReadOpException(std::string s) : ChunkOpException(s) {};
};

class ChunkMetaOpException : public ChunkOpException {
public:
    explicit ChunkMetaOpException(std::string s) : ChunkOpException(s) {};
};


template <class OperationType>
class ChunkOperation {

protected:
    const std::string path_; // chunk所在的目录

    std::vector<ABT_task> abt_tasks_; // 对文件进行操作的微线程
    std::vector<ABT_eventual> task_eventuals_; // 对微线程进行回调的eventuals

public:
    //  n为微线程数,写成这样既可以接受左值，也可以接收右值
    ChunkOperation(std::string path, size_t n) : path_(std::move(path)) {
        // 在构造实例前知道n很重要，因为不适合动态分配
        abt_tasks_.resize(n);
        task_eventuals_.resize(n);
    }

    // 默认n为1，即默认对一个块进行操作
    ChunkOperation(std::string& path) : ChunkOperation(path, 1) {};

    // 取消所有tasks和释放目前正在使用的资源
    void cancel_all_tasks() {
        GAOFS_DATA->spdlogger()->trace("{}() enter", __func__);
        // 释放所有微线程
        for(auto& task : abt_tasks_) {
            if(task) {
                ABT_task_cancel(task);
                ABT_task_free(&task);
            }
        }

        // 释放所有回调
        for(auto& eventual : task_eventuals_) {
            if(eventual) {
                ABT_eventual_reset(eventual);
                ABT_eventual_free(&eventual);
            }
        }

        abt_tasks_.clear();
        task_eventuals_.clear();
        static_cast<OperationType*>(this)->clear_task_args();
    }

    // 析构函数
    ~ChunkOperation() {
        cancel_all_tasks();
    }
};


class ChunkTruncateOperation : public ChunkOperation<ChunkTruncateOperation> {
    // 友元类，所以上面的才可以clear_task_arges,因为他是private
    friend class ChunkOperation<ChunkTruncateOperation>;

private:
    struct chunk_truncate_args {
        const std::string* path; // 被操作的快目录的路径
        size_t  size; // 整个文件需要截断的大小
        uint64_t host_id; // 当前结点的host_id,用于判断
        uint64_t host_size; // host集群的大小
        ABT_eventual eventual; // 关联的回调 ABT_eventual是一个指针
    };

    struct chunk_truncate_args task_arg_ {}; // 输入参数

    // 该函数由微线程使用， arg指向类型<chunk_truncate_args>的输入结构体的指针
    static void truncate_abt(void* _arg);

    // 重置task_arg_
    void clear_task_args();

public:
    // 构造函数
    explicit ChunkTruncateOperation(const std::string& path);

    // 析构函数
    ~ChunkTruncateOperation() = default;

    // 对外进行truncate的接口，有rpc处理函数调用，并启动一个非阻塞的微线程
    void truncate(size_t size, uint64_t host_id, uint64_t host_size);

    // 等待微线程执行，返回0为成功，否则失败
    int wait_for_task();
};

// 块操作类，用于每个写RPC请求具有一个这样的对象。可能涉及多个I/O任务，取决于所涉及的块的数量。
// 是可以说一个写操作对应一个类实例，并且一个类实例对应多个块
class ChunkWriteOperation : public ChunkOperation<ChunkWriteOperation> {
    friend class ChunkOperation<ChunkWriteOperation>;

private:
    struct chunk_write_args {
        const std::string* path; // 要读的文件的路径
        const char* buf; // buffer
        gaofs::rpc::chnk_id_t chnk_id; // chunk id
        size_t size; // 要写的内容的大小
        off64_t off; // 对单个chunk的写偏移
        ABT_eventual eventual; // 回调
    };

    std::vector<struct chunk_write_args> task_args_; // 因为是一对多个块，所以用vector存每个块对应的参数

    static void write_file_abt(void* _arg);

    void clear_task_args();

public:
    // 构造函数
    ChunkWriteOperation(const std::string& path, size_t n);

    // 析构函数
    ~ChunkWriteOperation() = default;

    // 由RPC处理器函数调用的请求，并启动一个非阻塞微线程
    // idx: rpc请求非阻塞写的数量
    void write_nonblock(size_t idx, uint64_t chunk_id, const char* bulk_buf_ptr, size_t size, off64_t offset);

    // int为0为成功，其他为出错， size_t为写的大小
    std::pair<int, size_t> wait_for_tasks();
};

// 和上面的写类似，多了一个bulk_args
class ChunkReadOperation : public ChunkOperation<ChunkReadOperation> {
    friend class ChunkOperation<ChunkReadOperation>;

private:
    struct chunk_read_args {
        const std::string* path;
        char* buf;
        gaofs::rpc::chnk_id_t chnk_id;
        off64_t off;
        ABT_eventual eventual;
        size_t size;
    };

    std::vector<struct chunk_read_args> task_args_;

    static void read_file_abt(void* _arg);

    void clear_task_args();

public:
    // 这个结构用于push read data去客户端
    struct bulk_args {
        margo_instance_id mid; // 服务端margo实例的ID
        hg_addr_t origin_addr; // 客户端抽象地址
        hg_bulk_t origin_bulk_handle; // 客户端的buf handle
        std::vector<size_t>* origin_offsets; // 客户端带来的偏移
        hg_bulk_t local_bulk_handle; // 本地bulk handle
        std::vector<size_t>* local_offsets; // 本地偏移
        std::vector<uint64_t>* chunk_ids; // 本次需要读的chunk_id
    };

    // 构造函数
    ChunkReadOperation(const std::string& path, size_t n);

    ~ChunkReadOperation() = default;

    void read_nonblock(size_t idx, uint64_t chunk_id, char* bulk_buf_ptr, size_t size, off64_t offset);

    std::pair<int, size_t> wait_for_tasks_and_push_back(const bulk_args& args);

};

} // namespace gaofs::data
#endif //GAOFS_DATA_HPP
