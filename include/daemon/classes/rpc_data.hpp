//
// Created by DELL on 2023/2/24.
//

#ifndef GAOFS_RPC_DATA_HPP
#define GAOFS_RPC_DATA_HPP

#include <common/rpc/distributor.hpp>
#include <memory>

extern "C" {
#include <abt.h>
#include <mercury.h>
#include <margo.h>
};

namespace gaofs {

namespace daemon {

class RPCData{

private:
    // 默认构造函数
    RPCData() {}

    // Margo id。它们还可以用于检索初始化时创建的Mercury类和上下文
    // margo_instance_id 其实就是指向margo_instance的指针
    margo_instance_id server_rpc_mid_;

    // Argobots线程池
    ABT_pool io_pool_;
    std::vector<ABT_xstream> io_streams_;
    std::string self_addr_str_;

    // Distributor
    std::shared_ptr<gaofs::rpc::Distributor> distributor_;

public:
    static RPCData* getInstance() {
        static RPCData instance;
        return &instance;
    }

    // 没有左值构造函数
    RPCData(RPCData const&) = delete;

    // 不能通过=左值构造
    void operator=(RPCData const&) = delete;

    // getter and setter
    margo_instance* server_rpc_mid();

    void server_rpc_mid(margo_instance* server_rpc_mid);

    ABT_pool io_pool() const;

    void io_pool(ABT_pool io_pool);

    std::vector<ABT_xstream>& io_streams();

    void io_streams(std::vector<ABT_xstream>& io_streams);

    const std::string& self_addr_str() const;

    void self_addr_str(const std::string& addr_str);

    const std::shared_ptr<gaofs::rpc::Distributor>& distributor() const;

    void distributor(const std::shared_ptr<gaofs::rpc::Distributor>& distributor);
};

} // namespace daemon

} // namespace gaofs

#endif //GAOFS_RPC_DATA_HPP
