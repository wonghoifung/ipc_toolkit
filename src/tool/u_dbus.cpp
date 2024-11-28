#include "g_dbusClient.h"
#include <glib.h>
#include "spdlogger.h"

static std::string dbusDaemonAddr = "/var/run/dbus/dbus_ipc_toolkit.sock";
static std::string clientBusNamePrefix = "ipc.toolkit.u.client";

#define CALL_TYPE "call"
#define PUB_TYPE  "publish"
#define SUB_TYPE  "subscribe"
static std::string type;

struct {
    std::string destBus;
    std::string method;
    std::string inputJson;
} method_arg;

struct {
    std::string signalName;
    std::string inputJson;
} publish_arg;

struct {
    std::string signalName;
} subscribe_arg;

static GMainLoop* loop = nullptr;

static void usage_exit() {
    // publish: 发布信号
    // subscribe: 订阅信号
    // call: 调用方法
    std::stringstream usagess;
    usagess << "usage: u_dbus call <dest-bus> <method> [input-json]" << std::endl;
    usagess << "       u_dbus publish <signal> [input-json]" << std::endl;
    usagess << "       u_dbus subscribe <signal>" << std::endl;
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

static void client_ready(void* ud);

static void start_loop() {
    loop = g_main_loop_new(NULL, FALSE);
    std::string clientBusName = clientBusNamePrefix + std::to_string(getpid());
    // log_info("clientBusName: {}", clientBusName);
    G_Simplified_DbusClient client(dbusDaemonAddr, clientBusName);
    if (!client.init(client_ready)) {
        g_main_loop_unref(loop);
        return ;
    }
    g_main_loop_run(loop);
    g_main_loop_unref(loop);
}

static void stop_loop() {
    if (loop != NULL) {
        g_main_loop_quit(loop);
    }
}

static void signal_callback(void*, const nlohmann::json& params) {
    std::cout << params.dump() << std::endl;
}

static void client_ready(void* ud) {
    G_Simplified_DbusClient* client = (G_Simplified_DbusClient*)ud;

    if (type == CALL_TYPE) {
        nlohmann::json output;
        client->call_method(method_arg.destBus, method_arg.method, method_arg.inputJson, output);
        std::cout << output.dump(4) << std::endl;
        stop_loop();
        return;
    }

    if (type == PUB_TYPE) {
        client->publish_signal(publish_arg.signalName, publish_arg.inputJson);
        stop_loop();
        return;
    }

    if (type == SUB_TYPE) {
        client->subscribe_signal(subscribe_arg.signalName, signal_callback);
        return;
    }

}

int main(int argc, char** argv) {
    G_Logger.Init("u_dbus.log", "/var/log/dbus");
    G_Logger.getSpdLogger()->set_level(spdlog::level::err);

    int argIdx = 1;
    type = get_next_arg_or_exit(argc, argv, argIdx++);

    if (type == CALL_TYPE) {
        method_arg.destBus = get_next_arg_or_exit(argc, argv, argIdx++);
        method_arg.method = get_next_arg_or_exit(argc, argv, argIdx++);
        method_arg.inputJson = get_next_arg_optional(argc, argv, argIdx++);
        start_loop();
        return 0;
    }

    if (type == PUB_TYPE) {
        publish_arg.signalName = get_next_arg_or_exit(argc, argv, argIdx++);
        publish_arg.inputJson = get_next_arg_optional(argc, argv, argIdx++);
        start_loop();
        return 0;
    }

    if (type == SUB_TYPE) {
        subscribe_arg.signalName = get_next_arg_or_exit(argc, argv, argIdx++);
        start_loop();
        return 0;
    }

    usage_exit();
}

