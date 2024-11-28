#include <stdbool.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include "dbusServer.h"
#include "dbusClient.h"
#include "spdlogger.h"

bool method1(void*, const nlohmann::json& input, DbusReply& reply) {
   log_info("method1 input: {}", input.dump());
   reply.code = 0;
   reply.msg = "success";
   reply.data["me"] = "method1";
   return true;
}
bool method2(void*, const nlohmann::json& input, DbusReply& reply) {
   log_info("method2 input: {}", input.dump());
   reply.code = 0;
   reply.msg = "success";
   reply.data["me"] = "method2";
   return true;
}
void signal1(void*, const nlohmann::json& input) {
   log_info("signal1 input: {}", input.dump());
}
void signal2(void*, const nlohmann::json& input) {
   log_info("signal2 input: {}", input.dump());
}
void listen(const char* my_bus_name) {
   DbusServer server("/var/run/dbus/dbus_ipc_toolkit.sock", my_bus_name);
   server.init();

   DbusInterface intf;
   DbusMethodHandlers& methodHandlers = intf.first;
   DbusSignalHandlers& signalHandlers = intf.second;
   methodHandlers["Method1"] = method1;
   methodHandlers["Method2"] = method2;
   signalHandlers["Signal1"] = signal1;
   signalHandlers["Signal2"] = signal2;
   server.add_interface("/test/service/object","test.service.interface",intf);

   server.run();
}

int main(int argc, char** argv) {
   G_Logger.Init("dbus_server.log", "/var/log/dbus");
   
   std::stringstream usagess;
   usagess << "usage: dbus_server listen <my-bus-name>" << std::endl;
   if (2 > argc) {
      std::cout << usagess.str() << std::endl;
      return 1;
   } 

   if (0 == strcmp(argv[1], "listen") && argc >= 3) {
      const char* my_bus_name = argv[2];
      listen(my_bus_name);
   }
   else {
      std::cout << usagess.str() << std::endl;
      return 1;
   }
   return 0;
} 
