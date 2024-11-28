#pragma once

#include "httplib.h"
#include "util.h"
#include <sys/socket.h>
#include <sys/un.h>

namespace httplib {

class UdsClient final : public ClientImpl {
public:
    explicit UdsClient(const std::string& unixsock):ClientImpl(unixsock) {
        set_address_family(AF_UNIX);
    }
    bool is_valid() const override {
        return true;
    }
private:
    bool create_and_connect_socket(Socket &socket, Error &error) override {
        int sockfd = ::socket(AF_UNIX, SOCK_STREAM, 0);
        if (sockfd == -1) {
            perror("socket");
            error = Error::Connection;
            return false;
        }

        struct sockaddr_un server_addr;
        server_addr.sun_family = AF_UNIX;
        strncpy(server_addr.sun_path, host_.c_str(), sizeof(server_addr.sun_path)-1);

        if (::connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
            perror("connect");
            close(sockfd);
            error = Error::Connection;
            return false;
        }

        socket.sock = sockfd;
        error = Error::Success;
        return true;
    }
    friend class ClientImpl;
};

} // namespace httplib

