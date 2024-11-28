#pragma once

#include "httplib.h"
#include "util.h"
#include <sys/socket.h>
#include <sys/un.h>

namespace httplib {

class UdsServer final : public httplib::Server {
public:
    UdsServer() {

    }
    ~UdsServer() override {

    }
    bool is_valid() const override {
        return true;
    }
    bool listen_unixsock(const std::string& unixsock) {
        set_address_family(AF_UNIX);

        int sock = socket(AF_UNIX, SOCK_STREAM, 0);
        if (sock == INVALID_SOCKET) {
            perror("socket");
            return false;
        }

        struct sockaddr_un addr;
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, unixsock.c_str(), sizeof(addr.sun_path)-1);

        if (::bind(sock, (struct sockaddr*)&addr, sizeof(addr)) == INVALID_SOCKET) {
            perror("bind");
            ::close(sock);
            return false;
        }

        ::listen(sock, SOMAXCONN);
        svr_sock_ = sock;

        return listen_after_bind();
    }
    bool remove_unixsock(const std::string& unixsock) {
        if (fileExists(unixsock)) {
            return removeFile(unixsock);
        }
        return true;
    }    
};

} // namespace httplib
