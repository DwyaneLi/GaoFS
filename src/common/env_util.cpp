//
// Created by DELL on 2022/12/23.
//

#include <string>
#include <common/env_util.hpp>
#include <cstdlib>

using namespace std;
string gaofs::env::get_var(const std::string &name, const std::string &default_value) {
    const char* const val = secure_getenv(name.c_str());
    return val != nullptr? string(val) : default_value;
}
