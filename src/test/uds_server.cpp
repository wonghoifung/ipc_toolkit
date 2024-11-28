#include <iostream>
#include "spdlogger.h"
#include "config.h"
#include <sys/socket.h>
#include <sys/un.h>
#include "udsServer.h"
#include "udsClient.h"

static void run_service() {
    httplib::UdsServer service;
    std::string unixsockpath = "/tmp/uds_server.sock";
    service.remove_unixsock(unixsockpath);
    service.Get("/test", [&](const httplib::Request& req, httplib::Response& res) {
        nlohmann::json body;
        body["code"] = 200;
        body["msg"] = "ok";
        res.set_content(body.dump(), "application/json");
    });
    service.Post("/test", [&](const httplib::Request& req, httplib::Response& res) {
        nlohmann::json body;
        body["code"] = 0;
        body["msg"] = "success";
        body["me"] = "Hello_method";
        res.set_content(body.dump(), "application/json");
    });
    service.listen_unixsock(unixsockpath);
}

int main() {
    if (!G_Config.Init("uds_server.ini","/etc/config")) {
        std::cout << "failed to init config" << std::endl;
        return 1;
    }

    G_Logger.Init("uds_server.log", "/var/log/uds_server");

    log_info("config beg<<<");
    for (auto i = G_Config.begin(); i != G_Config.end(); i++)
    {
        log_info("{}:{}", i->first, i->second);
    }
    log_info("config end>>>");

    run_service();

    return 0;
}

