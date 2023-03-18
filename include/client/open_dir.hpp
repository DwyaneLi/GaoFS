//
// Created by DELL on 2023/3/18.
//

#ifndef GAOFS_OPEN_DIR_HPP
#define GAOFS_OPEN_DIR_HPP

#include <string>
#include <vector>

#include <client/open_file_map.hpp>

namespace gaofs::filemap {

class DirEntry {

private:
    std::string name_;
    FileType type_;

public:
    DirEntry(const std::string& name, FileType type);

    const std::string& name();

    FileType type();
};

class OpenDir : public OpenFile {
private:
    std::vector<DirEntry> entries;

public:
    explicit OpenDir(const std::string& path);

    void add(const std::string& name, const FileType& type);

    const DirEntry& getdent(unsigned int pos);

    size_t size();
};

} // namespace gaofs::filemap

#endif //GAOFS_OPEN_DIR_HPP
