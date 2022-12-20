//
// Created by DELL on 2022/12/17.
//

#ifndef GAOFS_DISTRIBUTOR_H
#define GAOFS_DISTRIBUTOR_H

#include <config.hpp>
#include <vector>
#include <string>
#include <numeric>
#include <unordered_map>
#include <fstream>
#include <map>

// 这一部分和hash有关
namespace gaofs::rpc {

using chunkid_t = unsigned int;
using host_t = unsigned int;

// 一个抽象类Distributor
class Distributor {
public:
    // 获取本地host的编号
    virtual host_t localhost() const = 0;

    // 定位data在哪个节点上
    virtual host_t locate_data(const std::string& path, const chunkid_t& chunk_id) const = 0;

    // 定位data在哪个节点上，但是用于传递节点数量给server侧
    virtual host_t locate_data(const std::string& path, const chunkid_t& chunk_id, unsigned int hosts_size) = 0;

    // 定位元数据在哪个节点上
    virtual host_t locate_file_metadata(const std::string& path) const = 0;

    // 定位目录的元数据在哪个节点上（实际上是所有节点上）
    virtual std::vector<host_t> locate_directory_metadata(const std::string& path) const = 0;
};


// Distributor的子类，最基本的Distributor
class SimpleHashDistributor : public Distributor {
private:
    host_t localhost_; // 本地主机
    unsigned int hosts_size_{0}; // 主机数量
    std::vector<host_t> all_hosts_; // 主机列表
    std::hash<std::string> str_hash; // hash函数

public:
    // 构造函数
    SimpleHashDistributor();

    SimpleHashDistributor(host_t localhost, unsigned int hosts_size);

    host_t localhost() const override;

    host_t locate_data(const std::string& path, const chunkid_t& chunk_id) const override;

    host_t locate_data(const std::string& path, const chunkid_t& chunk_id, unsigned int host_size);

    host_t locate_file_metadata(const std::string& path) const override;

    std::vector<host_t> locate_directory_metadata(const std::string& path) const override;
};


} // namespace gaofs::rpc


#endif //GAOFS_DISTRIBUTOR_H
