//
// Created by DELL on 2023/4/1.
//

#ifndef GAOFS_DAEMON_ENV_HPP
#define GAOFS_DAEMON_ENV_HPP

#include <config.hpp>

#define ADD_PREFIX(str) COMMON_ENV_PREFIX str

namespace gaofs::env {

    static constexpr auto HOSTS_FILE = ADD_PREFIX("HOSTS_FILE");

} // namespace gaofs::env

#endif //GAOFS_DAEMON_ENV_HPP
