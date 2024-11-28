#include <stdbool.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include "dbusServer.h"
#include "dbusClient.h"
#include "spdlogger.h"

static std::string dbusDaemonAddr = "/var/run/dbus/dbus_ipc_toolkit.sock";
static std::string clientBusNamePrefix = "ipc.toolkit.client";

static void usage_exit() {
    // send: 发信号
    // call: 调用方法
    // introspect: 查看该bus提供的接口
    // listen_signal: 监听信号
    // listen_method: 监听方法
    std::stringstream usagess;
    usagess << "usage: dbus send <object> <interface> <signal> [input-json]" << std::endl;
    usagess << "       dbus call <dest-bus> <object> <interface> <method> [input-json]" << std::endl;
    usagess << "       dbus introspect <dest-bus>" << std::endl;
    usagess << "       dbus listen_signal <my-bus> <object> <interface> <signal>" << std::endl;
    usagess << "       dbus listen_method <my-bus> <object> <interface> <method>" << std::endl;
    std::cout << usagess.str();
    exit(1);
}

static const char* get_next_arg_or_exit(int argc, char** argv, int argIdx) {
    if (argc <= argIdx) usage_exit();
    return argv[argIdx];
}

static const char* get_next_arg_optional(int argc, char** argv, int argIdx) {
    if (argc <= argIdx) return "";
    return argv[argIdx];
}

static void send_signal(  
                        const std::string& object, const std::string& interface, 
                        const std::string& signalName, const std::string& inputJson) {
    std::string clientBusName = clientBusNamePrefix + std::to_string(getpid());
    DbusClient client(dbusDaemonAddr, clientBusName);
    client.init();
    nlohmann::json input;
    if (inputJson != "") {
        try {
            input = nlohmann::json::parse(inputJson);
        } catch (const std::exception& e) {
            log_err("dbus argument should be json:{}, exption:{}", inputJson, e.what());
            return;
        }
    }
    bool ret = client.sendSignal(object, interface, signalName, input);
    std::cout << signalName << " ret: " << std::boolalpha << ret << std::endl;
}

static void call_method(const std::string& destBus,
                        const std::string& object, const std::string& interface, 
                        const std::string& method, const std::string& inputJson) {
    std::string clientBusName = clientBusNamePrefix + std::to_string(getpid());
    DbusClient client(dbusDaemonAddr, clientBusName);
    client.init();
    nlohmann::json input,output;
    if (inputJson != "") {
        try {
            input = nlohmann::json::parse(inputJson);
        } catch (const std::exception& e) {
            log_err("dbus argument should be json:{}, exption:{}", inputJson, e.what());
            return;
        }
    }
    bool ret = client.callMethod(destBus, object, interface, method, input, output);
    std::cout << method << " ret: " << std::boolalpha << ret << std::endl;
    std::cout << "output: \n" << output.dump(4) << std::endl;
}

static void simple_signal(void*, const nlohmann::json& input) {
   log_info("signal input: {}", input.dump());
}

static void listen_signal(const std::string& myBus,
                        const std::string& object, const std::string& interface, 
                        const std::string& signalName) {
    DbusServer server(dbusDaemonAddr, myBus);
    server.init();

    DbusInterface intf;
    DbusSignalHandlers& signalHandlers = intf.second;
    signalHandlers[signalName] = simple_signal;
    server.add_interface(object,interface,intf);

    server.run();
}

static bool simple_method(void*, const nlohmann::json& input, DbusReply& reply) {
   log_info("method input: {}", input.dump());
   reply.code = 0;
   reply.msg = "success";
   reply.data["me"] = "dbus simple_method";
   return true;
}

static void listen_method(const std::string& myBus,
                        const std::string& object, const std::string& interface, 
                        const std::string& method) {
    DbusServer server(dbusDaemonAddr, myBus);
    server.init();

    DbusInterface intf;
    DbusMethodHandlers& methodHandlers = intf.first;
    methodHandlers[method] = simple_method;
    server.add_interface(object,interface,intf);

    server.run();
}

int main(int argc, char** argv) {
    G_Logger.Init("dbus.log", "/var/log/dbus");

    int argIdx = 1;
    std::string type = get_next_arg_or_exit(argc, argv, argIdx++);

    if (type == "introspect") {
        std::string destBus = get_next_arg_or_exit(argc, argv, argIdx++);
        call_method(destBus,"/ipc/toolkit","ipc.toolkit.DBus.Introspectable","Introspect","");
        return 0;
    }

    if (type == "listen_signal") {
        std::string myBus = get_next_arg_or_exit(argc, argv, argIdx++);
        std::string object = get_next_arg_or_exit(argc, argv, argIdx++);
        std::string interface = get_next_arg_or_exit(argc, argv, argIdx++);
        std::string signalName = get_next_arg_or_exit(argc, argv, argIdx++);
        listen_signal(myBus,object,interface,signalName);
        return 0;
    }

    if (type == "listen_method") {
        std::string myBus = get_next_arg_or_exit(argc, argv, argIdx++);
        std::string object = get_next_arg_or_exit(argc, argv, argIdx++);
        std::string interface = get_next_arg_or_exit(argc, argv, argIdx++);
        std::string method = get_next_arg_or_exit(argc, argv, argIdx++);
        listen_method(myBus,object,interface,method);
        return 0;
    }

    if (type == "send") {
        std::string object = get_next_arg_or_exit(argc, argv, argIdx++);
        std::string interface = get_next_arg_or_exit(argc, argv, argIdx++);
        std::string signalName = get_next_arg_or_exit(argc, argv, argIdx++);
        std::string inputJson = get_next_arg_optional(argc, argv, argIdx++);
        send_signal(object,interface,signalName,inputJson);
        return 0;
    }

    if (type == "call") {
        std::string destBus = get_next_arg_or_exit(argc, argv, argIdx++);
        std::string object = get_next_arg_or_exit(argc, argv, argIdx++);
        std::string interface = get_next_arg_or_exit(argc, argv, argIdx++);
        std::string method = get_next_arg_or_exit(argc, argv, argIdx++);
        std::string inputJson = get_next_arg_optional(argc, argv, argIdx++);
        call_method(destBus,object,interface,method,inputJson);
        return 0;
    }
    
    usage_exit();
}

