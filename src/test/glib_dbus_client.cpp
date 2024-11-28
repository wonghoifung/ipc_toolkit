#include "g_dbusClient.h"
#include "spdlogger.h"
#include <glib.h>
#include "g_timer.h"

void signal3(void*, const nlohmann::json& params) {
    log_info("signal3: {}", params.dump());
}

void signal4(void*, const nlohmann::json& params) {
    log_info("signal4: {}", params.dump());
}

void client_ready(void* ud) {
    log_info("client ready");
    
    G_DbusClient* client = (G_DbusClient*)ud;
    
    nlohmann::json input1,output1;
    input1["key"] = 123;
    client->call_method("org.example.ExampleService", "/org/example/ExampleObject", "org.example.ExampleInterface", "Hello", input1, output1);
    log_info("Hello res: {}", output1.dump());

    nlohmann::json input2,output2;
    input2["key"] = 456;
    client->call_method("org.example.ExampleService", "/org/example/ExampleObject", "org.example.ExampleInterface", "World", input2, output2);
    log_info("World res: {}", output2.dump());

    nlohmann::json input3;
    input3["key"] = 789;
    client->publish_signal("/org/example/ExampleObject", "org.example.ExampleInterface", "Signal1", input3);

    nlohmann::json input4;
    input4["key"] = 012;
    client->publish_signal("/org/example/ExampleObject", "org.example.ExampleInterface", "Signal2", input4);

    client->subscribe_signal("/org/example/ExampleObject", "org.example.ExampleInterface", "Signal3", signal3);
    client->subscribe_signal("/org/example/ExampleObject", "org.example.ExampleInterface", "Signal4", signal4);

}

void timeout(G_Timer* timer) {
    G_DbusClient* client = (G_DbusClient*)(timer->get_user_data());

    nlohmann::json input1,output1;
    input1["key"] = 123;
    client->call_method("org.example.ExampleService", "/org/example/ExampleObject", "org.example.ExampleInterface", "Hello", input1, output1);
    log_info("Hello res: {}", output1.dump());

    nlohmann::json input2,output2;
    input2["key"] = 456;
    client->call_method("org.example.ExampleService", "/org/example/ExampleObject", "org.example.ExampleInterface", "World", input2, output2);
    log_info("World res: {}", output2.dump());

    nlohmann::json input3;
    input3["key"] = 111789;
    client->publish_signal("/org/example/ExampleObject", "org.example.ExampleInterface", "Signal1", input3);

    nlohmann::json input4;
    input4["key"] = 222012;
    client->publish_signal("/org/example/ExampleObject", "org.example.ExampleInterface", "Signal2", input4);
}

int main() {
    G_Logger.Init("g_dbus_client.log", "/var/log/dbus");
    GMainLoop* loop = g_main_loop_new(NULL, FALSE);

    G_DbusClient client("/var/run/dbus/dbus_ipc_toolkit.sock", "org.example.ExampleClient");
    if (!client.init(client_ready)) {
        return 1;
    }

    G_Timer gtimer;
    gtimer.start_cycle(3000, timeout, &client);

    g_main_loop_run(loop);
    g_main_loop_unref(loop);
}

