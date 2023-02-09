//
// Created by DELL on 2023/1/16.
//

#include <daemon/daemon.hpp>
#include <filesystem>
#include <iostream>
#include <daemon/ops/metadentry.hpp>
#include <tuple>

namespace fs = std::filesystem;

void init_environment_test() {
    std::string metadata_path = "/home/dwyaneli/gaofs_metadata";
    fs::create_directories(metadata_path);

    // 初始化数据库
    GAOFS_DATA->mdb(std::make_shared<gaofs::metadata::MetadataDB>(metadata_path, GAOFS_DATA->dbbackend()));

    // 根据config初始化选项
    GAOFS_DATA->atime_state(gaofs::config::metadata::use_atime);
    GAOFS_DATA->ctime_state(gaofs::config::metadata::use_ctime);
    GAOFS_DATA->mtime_state(gaofs::config::metadata::use_mtime);
    GAOFS_DATA->link_cnt_state(gaofs::config::metadata::use_link_cnt);
    GAOFS_DATA->blocks_state(gaofs::config::metadata::use_blocks);

    // 为根目录创建原条目, 同时也在测试create
    gaofs::metadata::Metadata root_md{S_IFDIR | S_IRWXU | S_IRWXG | S_IRWXO};
    gaofs::metadata::create("/", root_md);

    gaofs::metadata::Metadata _m_md;
    _m_md.size(500);
    _m_md.mode(S_IFDIR | S_IRWXU | S_IRWXG | S_IRWXO);
    gaofs::metadata::create("/_m", _m_md);

    gaofs::metadata::Metadata _m_m_md;
    _m_m_md.size(600);
    _m_m_md.mode(S_IFDIR | S_IRWXU | S_IRWXG | S_IRWXO);
    gaofs::metadata::create("/_m_m", _m_m_md);

    gaofs::metadata::Metadata _m_1_md;
    _m_1_md.size(700);
    _m_1_md.mode(S_IRWXU | S_IRWXG | S_IRWXO);
    gaofs::metadata::create("/_m/1", _m_1_md);

    gaofs::metadata::Metadata _m_2_md;
    _m_2_md.size(800);
    _m_2_md.mode(S_IRWXU | S_IRWXG | S_IRWXO);
    gaofs::metadata::create("/_m/2", _m_2_md);

    gaofs::metadata::Metadata _m_m_1_md;
    _m_m_1_md.size(900);
    _m_m_1_md.mode(S_IRWXU | S_IRWXG | S_IRWXO);
    gaofs::metadata::create("/_m_m/1", _m_m_1_md);

    gaofs::metadata::Metadata _m_m_2_md;
    _m_m_2_md.size(1000);
    _m_m_2_md.mode(S_IRWXU | S_IRWXG | S_IRWXO);
    gaofs::metadata::create("/_m_m/2", _m_m_2_md);

    // 测试get_size and get
    std::cout << "----------------------------------test get_size and get---------------------------------------" << std::endl;
    auto _m_size = gaofs::metadata::get_size("/_m");
    auto _m_1_size = gaofs::metadata::get_size("/_m/1");
    auto _m_2_size = gaofs::metadata::get_size("/_m/2");
    auto _m_m_size = gaofs::metadata::get_size("/_m_m");
    auto _m_m_1_size = gaofs::metadata::get_size("/_m_m/1");
    auto _m_m_2_size = gaofs::metadata::get_size("/_m_m/2");
    std::cout << "/_m: " << _m_size << "\n"
              << "/_m/1: " << _m_1_size << "\n"
              << "/_m/2: " << _m_2_size << "\n"
              << "/_m_m: " << _m_m_size << "\n"
              << "/_m_m/1" << _m_m_1_size << "\n"
              << "/_m_m/2" << _m_m_2_size << "\n\n" << std::endl;


    // 测试update
    std::cout << "----------------------------------update---------------------------------------" << std::endl;
    _m_md.size(1500);
    _m_m_md.size(1600);
    _m_1_md.size(1700);
    _m_2_md.size(1800);
    _m_m_1_md.size(1900);
    _m_m_2_md.size(2000);
    gaofs::metadata::update("/_m", _m_md);
    gaofs::metadata::update("/_m_m", _m_m_md);
    gaofs::metadata::update("/_m/1", _m_1_md);
    gaofs::metadata::update("/_m/2", _m_2_md);
    gaofs::metadata::update("/_m_m/1", _m_m_1_md);
    gaofs::metadata::update("/_m_m/2", _m_m_2_md);
    _m_size = gaofs::metadata::get_size("/_m");
    _m_1_size = gaofs::metadata::get_size("/_m/1");
    _m_2_size = gaofs::metadata::get_size("/_m/2");
    _m_m_size = gaofs::metadata::get_size("/_m_m");
    _m_m_1_size = gaofs::metadata::get_size("/_m_m/1");
    _m_m_2_size = gaofs::metadata::get_size("/_m_m/2");
    std::cout << "/_m: " << _m_size << "\n"
              << "/_m/1: " << _m_1_size << "\n"
              << "/_m/2: " << _m_2_size << "\n"
              << "/_m_m: " << _m_m_size << "\n"
              << "/_m_m/1" << _m_m_1_size << "\n"
              << "/_m_m/2" << _m_m_2_size << "\n\n" << std::endl;

    // 测试update size
    std::cout << "----------------------------------update size---------------------------------------" << std::endl;
    gaofs::metadata::update_size("/_m", 1000, 0, true);
    gaofs::metadata::update_size("/_m_m", 1000, 0, true);
    gaofs::metadata::update_size("/_m/1", 1000, 0, true);
    gaofs::metadata::update_size("/_m/2", 1000, 0, true);
    gaofs::metadata::update_size("/_m_m/1", 1000, 0, true);
    gaofs::metadata::update_size("/_m_m/2", 1000, 0, true);
    _m_size = gaofs::metadata::get_size("/_m");
    _m_1_size = gaofs::metadata::get_size("/_m/1");
    _m_2_size = gaofs::metadata::get_size("/_m/2");
    _m_m_size = gaofs::metadata::get_size("/_m_m");
    _m_m_1_size = gaofs::metadata::get_size("/_m_m/1");
    _m_m_2_size = gaofs::metadata::get_size("/_m_m/2");
    std::cout << "/_m: " << _m_size << "\n"
              << "/_m/1: " << _m_1_size << "\n"
              << "/_m/2: " << _m_2_size << "\n"
              << "/_m_m: " << _m_m_size << "\n"
              << "/_m_m/1" << _m_m_1_size << "\n"
              << "/_m_m/2" << _m_m_2_size << "\n\n" << std::endl;

    // 测试remove
    std::cout << "----------------------------------remove---------------------------------------" << std::endl;
    gaofs::metadata::remove("/_m_m");
    std::cout << "/_m_m: " << gaofs::metadata::get_size("/_m_m") << "\n" << std::endl;
    _m_m_md.size(2600);
    gaofs::metadata::create("/_m_m", _m_m_md);

    // 测试create_first_chunk
    gaofs::metadata::First_chunk _m_1_f;
    _m_1_f.size(700);
    _m_1_f.content("this is /_m/1");
    gaofs::metadata::create_first_chunk("/_m/1", _m_1_f);
    gaofs::metadata::First_chunk _m_2_f;
    _m_2_f.size(800);
    _m_2_f.content("this is /_m/2");
    gaofs::metadata::create_first_chunk("/_m/2", _m_2_f);

    gaofs::metadata::Metadata _f_md;
    _f_md.size(2100);
    _f_md.mode(S_IRWXU | S_IRWXG | S_IRWXO);
    gaofs::metadata::create("/_f", _f_md);
    gaofs::metadata::First_chunk _f_f;
    _f_f.size(900);
    _f_f.content("this is /_f");
    gaofs::metadata::create_first_chunk("/_f", _f_f);

    // 测试get_first_chunk
    std::cout << "----------------------------------test get_size and get(first chunk)---------------------------------------" << std::endl;
    auto _m_1_f_size = gaofs::metadata::get_size_first_chunk("/_m/1");
    auto _m_2_f_size = gaofs::metadata::get_size_first_chunk("/_m/2");
    auto _f_f_size = gaofs::metadata::get_size_first_chunk("/_f");
    std::cout << "/_m/1 firstchunk : " << _m_1_f_size << "\n"
              << "/_m/2 firstchunk : " << _m_2_f_size << "\n"
              << "/_f firstchunk : " << _f_f_size << "\n\n" << std::endl;


    // 测试get_first_chunk_content
    std::cout << "----------------------------------test get_content and get(first_chunk)---------------------------------------" << std::endl;
    auto _m_1_f_c = gaofs::metadata::get_content_first_chunk("/_m/1");
    auto _m_2_f_c = gaofs::metadata::get_content_first_chunk("/_m/2");
    auto _f_f_c = gaofs::metadata::get_content_first_chunk("/_f");
    std::cout << "/_m/1 firstchunk : " << _m_1_f_c << "\n"
              << "/_m/2 firstchunk : " << _m_2_f_c << "\n"
              << "/_f firstchunk : " << _f_f_c << "\n\n" << std::endl;

    // 测试update_first_chunk
    std::cout << "----------------------------------test update(first_chunk)---------------------------------------" << std::endl;
    _m_1_f.size(1700);
    _m_2_f.size(1800);
    _f_f.size(1900);
    gaofs::metadata::update_first_chunk("/_m/1", _m_1_f);
    gaofs::metadata::update_first_chunk("/_m/2", _m_2_f);
    gaofs::metadata::update_first_chunk("/_f", _f_f);
    _m_1_f_size = gaofs::metadata::get_size_first_chunk("/_m/1");
    _m_2_f_size = gaofs::metadata::get_size_first_chunk("/_m/2");
    _f_f_size = gaofs::metadata::get_size_first_chunk("/_f");
    std::cout << "/_m/1 firstchunk : " << _m_1_f_size << "\n"
              << "/_m/2 firstchunk : " << _m_2_f_size << "\n"
              << "/_f firstchunk : " << _f_f_size << "\n\n" << std::endl;

    // 测试remove_first_chunk
    std::cout << "----------------------------------test remove(first_chunk)---------------------------------------" << std::endl;
    gaofs::metadata::remove_first_chunk("/_m/1");
    std::cout << "/_m/1 firstchunk : " << gaofs::metadata::get_size_first_chunk("/_m/1");
    _m_1_f.size(700);
    _m_1_f.content("this is /_m/1");
    gaofs::metadata::create_first_chunk("/_m/1", _m_1_f);

    // 测试get_dirents
    std::cout << "----------------------------------test get dirents---------------------------------------" << std::endl;
    auto root_dirents = gaofs::metadata::get_dirents("/");
    auto _m_dirents = gaofs::metadata::get_dirents("/_m");
    auto _m_m_dirents = gaofs::metadata::get_dirents("/_m_m");
    std::cout << "/:" << std::endl;
    for (int i = 0; i < root_dirents.size(); i++) {
        std::cout << root_dirents[i].first << " " << root_dirents[i].second << std::endl;
    }
    std::cout << "/_m:" << std::endl;
    for (int i = 0; i < _m_dirents.size(); i++) {
        std::cout << _m_dirents[i].first << " " << _m_dirents[i].second << std::endl;
    }
    std::cout << "/_m_m:" << std::endl;
    for (int i = 0; i < _m_m_dirents.size(); i++) {
        std::cout << _m_m_dirents[i].first << " " << _m_m_dirents[i].second << std::endl;
    }

    // 测试get_dirents_extended
    std::cout << "----------------------------------test get dirents_extened---------------------------------------" << std::endl;
    auto root_dirents_e = gaofs::metadata::get_dirents_extended("/");
    auto _m_dirents_e = gaofs::metadata::get_dirents_extended("/_m");
    auto _m_m_dirents_e = gaofs::metadata::get_dirents_extended("/_m_m");
    std::cout << "/:" << std::endl;
    for (int i = 0; i < root_dirents_e.size(); i++) {
        std::cout << std::get<0>(root_dirents_e[i]) << " " << std::get<2>(root_dirents_e[i]) << std::endl;
    }
    std::cout << "/_m:" << std::endl;
    for (int i = 0; i < _m_dirents_e.size(); i++) {
        std::cout << std::get<0>(_m_dirents_e[i]) << " " << std::get<2>(_m_dirents_e[i]) << std::endl;
    }
    std::cout << "/_m_m:" << std::endl;
    for (int i = 0; i < _m_m_dirents_e.size(); i++) {
        std::cout << std::get<0>(_m_m_dirents_e[i]) << " " << std::get<2>(_m_m_dirents_e[i]) << std::endl;
    }
    std::cout << "\n" << std::endl;

}

int main() {
    init_environment_test();
    return 0;
}

