//
// Created by DELL on 2023/3/13.
//

#ifndef GAOFS_ENV_HPP
#define GAOFS_ENV_HPP

#include <config.hpp>

#define ADD_PREFIX(str) CLIENT_ENV_PREFIX str

// gaofs的环境变量
namespace gaofs::env {

    static constexpr auto LOG = ADD_PREFIX("LOG");
    // GAOFS_DEBUG_BUILD
#ifdef GAOFS_DEBUG_BUILD
    static constexpr auto LOG_DEBUG_VERBOSITY = ADD_PREFIX("LOG_DEBUG_VERBOSITY");
    static constexpr auto LOG_SYSCALL_FILTER = ADD_PREFIX("LOG_SYSCALL_FILTER");
#endif

    static constexpr auto LOG_OUTPUT = ADD_PREFIX("LOG_OUTPUT");
    static constexpr auto LOG_OUTPUT_TRUNC = ADD_PREFIX("LOG_OUTPUT_TRUNC");
    static constexpr auto CWD = ADD_PREFIX("CWD");
    static constexpr auto HOSTS_FILE = ADD_PREFIX("HOSTS_FILE");
#ifdef GAOFS_ENABLE_FORWARDING
    static constexpr auto FORWARDING_MAP_FILE = ADD_PREFIX("FORWARDING_MAP_FILE");
#endif

} // namespace gaofs::env

#undef ADD_PREFIX

#endif //GAOFS_ENV_HPP
