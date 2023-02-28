//
// Created by DELL on 2023/2/24.
//

#include <daemon/classes/rpc_data.hpp>

using namespace std;

namespace gaofs {

namespace daemon {

// getter and setter
margo_instance * RPCData::server_rpc_mid() {
    return server_rpc_mid_;
}

void RPCData::server_rpc_mid(margo_instance *server_rpc_mid) {
    server_rpc_mid_ = server_rpc_mid;
}

ABT_pool RPCData::io_pool() const {
    return io_pool_;
}

void RPCData::io_pool(ABT_pool io_pool) {
    io_pool_ = io_pool;
}

const std::string & RPCData::self_addr_str() const {
    return self_addr_str_;
}

void RPCData::self_addr_str(const string &addr_str) {
    self_addr_str_ = addr_str;
}

const std::shared_ptr<gaofs::rpc::Distributor>& RPCData::distributor() const {
    return distributor_;
}

void RPCData::distributor(const shared_ptr<gaofs::rpc::Distributor> &distributor) {
    distributor_ = distributor;
}

} // namespace daemon

} // namespace gaofs

