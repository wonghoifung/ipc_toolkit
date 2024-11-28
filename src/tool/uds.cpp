#include "udsClient.h"

int main(int argc, char** argv) {
    if (argc < 4) {
        std::cout << "usage: uds <unix-domain-socket-path> <get|post> <api> [param]" << std::endl;
        return 1;
    }
    char* uds_path = argv[1];
    std::string method_type = argv[2];
    char* api = argv[3];
    char* param = (argc > 4 ? argv[4] : nullptr);
    std::cout << ">>> uds path:    " << uds_path << std::endl;
    std::cout << ">>> method type: " << method_type << std::endl;
    std::cout << ">>> api:         " << api << std::endl;
    std::cout << ">>> param:       " << (param == nullptr ? "" : param) << std::endl;

    httplib::UdsClient client(uds_path);
    httplib::Headers headers = {{"Host", "localhost"}};
    httplib::Result res;
    if (method_type == "get") {
        res = client.Get(api, headers);
    } else if (method_type == "post") {
        if (param) {
            res = client.Post(api, headers, param, strlen(param), "application/json");
        } else {
            res = client.Post(api, headers);
        }
    } else {
        std::cerr << "error method type, should be get/post." << std::endl;
        return 1;
    }

    if (res) {
        if (res->status == 200) {
            std::cout << "response: " << res->body << std::endl;
        } else {
            std::cerr << "request failed with status code: " << res->status << std::endl;
        }
    } else {
        std::cerr << "request failed" << std::endl;
        return 1;
    }

    return 0;
}

