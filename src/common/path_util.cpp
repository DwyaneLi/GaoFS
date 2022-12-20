//
// Created by DELL on 2022/12/18.
//

#include<common/path_util.hpp>

#include <cstring>
#include <system_error>
#include <cassert>

using namespace std;

namespace gaofs::path {

// 是否为相对路径
bool is_relative(const string& path) {
    return (!path.empty()) && (path.front() != separator);
}

// 是否为绝对路径
bool is_absolute(const string& path) {
    return (!path.empty()) && (path.front() == separator);
}

// 是否有末尾斜杠
bool has_trailing_slash(const string& path) {
    return (!path.empty()) && (path.back() == separator);
}

// 组合
string prepend_path(const string& prefix_path, const char* raw_path) {
    assert(!has_trailing_slash(prefix_path));
    size_t raw_len = strlen(raw_path);
    string res;
    res.reserve(prefix_path.size() + 1 + raw_len);
    res.append(prefix_path);
    res.push_back(separator);
    res.append(raw_path,raw_len);
    return res;
}

// 分隔路径
vector<string> split_path(const string& path) {
    vector<string> tokens;
    size_t start = string::npos;
    size_t end = (path.front() == separator)? 1: 0;

    while(end != string::npos && end < path.size()) {
        start = end;
        end = path.find(separator, start);
        tokens.push_back(path.substr(start, end - start));
        if(end != string::npos) {
            end++;
        }
    }

    return tokens;
}

// 绝对路径转相对路径
string absolute_to_relative(const string& root_path, const string& absolute_path) {
    assert(is_absolute(root_path));
    assert(is_absolute(absolute_path));
    assert(!has_trailing_slash(root_path));

    auto diff_its = mismatch(absolute_path.cbegin(), absolute_path.cend(), root_path.cbegin());
    if(diff_its.second != root_path.cend()) {
        // 表明root_path完全不是absolute_path的前缀
        return {};
    }

    // relative_path的开始和结束
    auto rel_it_begin = diff_its.first;
    auto rel_it_end = absolute_path.cend();
    assert((size_t)(rel_it_begin - absolute_path.cbegin()) == root_path.size());
    if(rel_it_begin == rel_it_end) {
        // root_path == absolute_path
        return {'/'};
    }

    // 删除relative_path末尾的’/‘
    if(has_trailing_slash(absolute_path) && rel_it_begin != rel_it_end - 1) {
        rel_it_end--;
    }

    return {rel_it_begin, rel_it_end};
}

// 返回目录的路径
string dirname(const string& path) {
    assert(path.size() > 1 || path.front() == separator);
    assert(path.size() == 1 || !has_trailing_slash(path));

    auto parent_path_size = path.find_last_of(separator);
    assert(parent_path_size != string::npos);
    if(parent_path_size == 0) {
        // parent is '/'
        parent_path_size = 1;
    }
    return path.substr(0, parent_path_size);
}

} // namespace gaofs::path
