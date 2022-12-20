//
// Created by DELL on 2022/12/19.
//

#ifndef GAOFS_METADATA_HPP
#define GAOFS_METADATA_HPP
#pragma once //指定在创建过程中该编译指示所在的文件仅仅被编译程序包含（打开）一次

#include <config.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string>

namespace gaofs::metadata {

constexpr mode_t LINK_MODE = ((S_IRWXU | S_IRWXG | S_IRWXO) | S_IFLNK );

class Metadata {
private:
    time_t atime_{}; // 访问时间
    time_t mtime_{}; // 修改时间(文件内容)
    time_t ctime_{}; // 改变时间(文件实现和内容)
    mode_t mode_{}; // 文件属性位掩码的类型
    nlink_t link_count_{}; // inode硬连接的数量
    size_t size_{}; // 文件大小
    blkcnt_t blocks_{}; // 文件块的数量
#ifdef HAS_SYMLINKS
    std::string target_path_; // For links this is the path of the target file 对于链接，这是目标文件的路径
#endif

public:
    Metadata() = default;

    explicit Metadata(mode_t mode); // explicit禁止隐式转换

#ifdef HAS_SYMLINKS
    Metadata(mode_t mode, const std::string& target_path);
#endif

    // 利用二进制序初始化metadata
    explicit Metadata(const std::string& binary_str);

    // 把metadata序列化，这样用于保存到rocksdb里
    std::string serialize() const;

    // 初始化三个时间
    void init_ACM_time();

    // 修改时间，哪个为真修改哪个
    void update_ACM_time(bool a, bool c, bool m);

    // Getter and Setter
    time_t atime() const;

    void atime(time_t atime_);

    time_t mtime() const;

    void mtime(time_t mtime_);

    time_t ctime() const;

    void ctime(time_t ctime_);

    mode_t mode() const;

    void mode(mode_t mode_);

    nlink_t link_count() const;

    void link_count(nlink_t link_count_);

    size_t size() const;

    void size(size_t size_);

    blkcnt_t blocks() const;

    void blocks(blkcnt_t blocks_);

#ifdef HAS_SYMLINKS
    std::string target_path() const;

    void target_path(const std::string& target_path);

    bool is_link() const;
#endif
};

} // gaofs::metadata
#endif //GAOFS_METADATA_HPP
