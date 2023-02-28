//
// Created by DELL on 2023/2/27.
//

#include <common/rpc/rpc_util.hpp>

extern "C" {
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
}

using namespace std;

namespace gaofs::rpc {

hg_bool_t bool_to_merc_bool(bool state) {
    return (state? static_cast<hg_bool_t >(HG_TRUE): static_cast<hg_bool_t >(HG_FALSE));
}

// 获取当前主机的ip
string get_my_hostname(bool short_name) {
    char hostname[1024];
    auto ret = gethostname(hostname, 1024);
    if(ret == 0) {
        string hostname_s(hostname);
        if(!short_name) {
            return hostname_s;
        }

        //获取short name
        auto pos = hostname_s.find("."s);
        if(pos != string::npos) {
            hostname_s = hostname_s.substr(0, pos);
        }
        return hostname_s;
    } else {
        return ""s;
    }
}

}