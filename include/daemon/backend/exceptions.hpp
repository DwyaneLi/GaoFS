//
// Created by DELL on 2022/12/24.
//

#ifndef GAOFS_EXCEPTIONS_HPP
#define GAOFS_EXCEPTIONS_HPP

#include <string>
#include <stdexcept>

namespace gaofs::metadata {

// 父类，继承runtime_error
class DBException : public std::runtime_error {
public:
    explicit DBException(const std::string &s) : std::runtime_error(s) {};
};

// 子类1：Not Found异常
class NotFoundException : public DBException {
public:
    explicit NotFoundException(const std::string& s) : DBException(s) {};
};

// 子类2：已存在异常
class ExistsException : public DBException {
public:
    explicit ExistsException(const std::string& s) : DBException(s) {};
};

} // namespace gaofs::metadata

#endif //GAOFS_EXCEPTIONS_HPP
