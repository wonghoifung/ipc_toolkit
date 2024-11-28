#include "g_dbusServer.h"
#include "spdlogger.h"
#include <glib.h>
#include "g_timer.h"

bool Hello_method(void*, const nlohmann::json& input, DbusReply& reply) {
   log_info("Hello_method input: {}", input.dump());
   reply.code = 0;
   reply.msg = "success";
   reply.data["me"] = "Hello_method";
   return true;
}
bool World_method(void*, const nlohmann::json& input, DbusReply& reply) {
   log_info("World_method input: {}", input.dump());
   reply.code = 0;
   reply.msg = "success";
   reply.data["me"] = "World_method";
   return true;
}
void signal1(void*, const nlohmann::json& input) {
   log_info("signal1 input: {}", input.dump());
}
void signal2(void*, const nlohmann::json& input) {
   log_info("signal2 input: {}", input.dump());
}
void timeout(G_Timer* timer) {
   G_Simplified_DbusServer* server = (G_Simplified_DbusServer*)(timer->get_user_data());

   nlohmann::json input3;
   input3["key"] = 789;
   server->publish_signal("Signal3", input3);

   nlohmann::json input4;
   input4["key"] = 012;
   server->publish_signal("Signal4", input4);
}

int main() {
   G_Logger.Init("g_dbus_server.log", "/var/log/dbus");
   GMainLoop* loop = g_main_loop_new(NULL, FALSE);

   G_Simplified_DbusServer server("/var/run/dbus/dbus_ipc_toolkit.sock", "org.example.ExampleService");
   server.init();

   DbusInterface intf;
   DbusMethodHandlers& methodHandlers = intf.first;
   DbusSignalHandlers& signalHandlers = intf.second;
   methodHandlers["Hello"] = Hello_method;
   methodHandlers["World"] = World_method;
   signalHandlers["Signal1"] = signal1;
   signalHandlers["Signal2"] = signal2;
   server.add_interface(intf);

   server.start();

   G_Timer gtimer;
   gtimer.start_cycle(3000, timeout, &server);

   g_main_loop_run(loop);
   g_main_loop_unref(loop);
}



