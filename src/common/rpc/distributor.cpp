//
// Created by DELL on 2022/12/17.
//

#include <common/rpc/distributor.hpp>

using namespace std;

namespace gaofs {

namespace rpc {

// 空构造函数
SimpleHashDistributor::SimpleHashDistributor() {}

// 初始化构造函数
SimpleHashDistributor::SimpleHashDistributor(host_t localhost, unsigned int hosts_size)
    : localhost_(localhost), hosts_size_(hosts_size), all_hosts_(hosts_size){
    iota(all_hosts_.begin(), all_hosts_.end(), 0);
}

// 获取本地节点号
host_t SimpleHashDistributor::localhost() const {
    return localhost_;
}

// 定位数据块在哪个结点
host_t SimpleHashDistributor::locate_data(const std::string &path, const chunkid_t &chunk_id) const {
    return str_hash(path + to_string(chunk_id)) % hosts_size_;
}

// 定位数据块在哪个结点(包括hosts_size_的初始化)
host_t SimpleHashDistributor::locate_data(const std::string &path, const chunkid_t &chunk_id, unsigned int host_size) {
    if(hosts_size_ != host_size) {
        hosts_size_ = host_size;
        all_hosts_ = vector<host_t>(host_size);
        iota(all_hosts_.begin(), all_hosts_.end(), 0);
    }

    return str_hash(path + to_string(chunk_id)) % hosts_size_;
}

// 定位文件元数据
host_t SimpleHashDistributor::locate_file_metadata(const std::string &path) const {
    return str_hash(path) % hosts_size_;
}

// 定位目录元数据
vector<host_t> SimpleHashDistributor::locate_directory_metadata(const std::string &path) const {
    return all_hosts_;
}

}// gaofs::rpc

}// gaofs