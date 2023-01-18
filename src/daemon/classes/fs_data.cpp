//
// Created by DELL on 2023/1/16.
//

#include <daemon/classes/fs_data.hpp>

namespace gaofs::daemon {

// getter and setter

const std::shared_ptr<spdlog::logger>& FsData::spdlogger() const {
    return spdlogger_;
}

void FsData::spdlogger(const std::shared_ptr<spdlog::logger>& spdlogger) {
    spdlogger_ = spdlogger;
}

const std::shared_ptr<gaofs::metadata::MetadataDB> & FsData::mdb() const {
    return mdb_;
}

void FsData::mdb(const std::shared_ptr<gaofs::metadata::MetadataDB> &mdb) {
    mdb_ = mdb;
}

void FsData::close_mdb() {
    mdb_.reset();
}

const std::string& FsData::rootdir() const {
    return rootdir_;
}

void FsData::rootdir(const std::string &rootdir) {
    rootdir_ = rootdir;
}

const std::string& FsData::rootdir_suffix() const {
    return rootdir_suffix_;
}

void FsData::rootdir_suffix(const std::string &rootdir_suffix) {
    rootdir_suffix_ = rootdir_suffix;
}

const std::string& FsData::mountdir() const {
    return mountdir_;
}

void FsData::mountdir(const std::string &mountdir) {
    mountdir_ = mountdir;
}

const std::string& FsData::metadir() const {
    return metadir_;
}

void FsData::metadir(const std::string &metadir) {
    metadir_ = metadir;
}

std::string_view FsData::dbbackend() const {
    return dbbackend_;
}

void FsData::dbbackend(const std::string &dbbackend) {
    dbbackend_ = dbbackend;
}

bool FsData::atime_state() const {
    return atime_state_;
}

void FsData::atime_state(bool atime_state) {
    atime_state_ = atime_state;
}

bool FsData::mtime_state() const {
    return mtime_state_;
}

void FsData::mtime_state(bool mtime_state) {
    mtime_state_ = mtime_state;
}

bool FsData::ctime_state() const {
    return ctime_state_;
}

void FsData::ctime_state(bool ctime_state) {
    ctime_state_ = ctime_state;
}

bool FsData::link_cnt_state() const {
    return link_cnt_state_;
}

void FsData::link_cnt_state(bool link_cnt_state) {
    link_cnt_state_ = link_cnt_state;
}

bool FsData::blocks_state() const {
    return blocks_state_;
}

void FsData::blocks_state(bool blocks_state) {
    blocks_state_ = blocks_state;
}


// TODO: Parallax

// TODO: Statistics

// TODO: Prometheus

// TODO: RPC management

} // namespace gaofs::daemon
