#include "dbusCommon.h"
#include "spdlogger.h"
#include <stdlib.h>

bool set_dbus_addr_env(const std::string& dbus_addr) {
    std::string dbus_session_bus_address = "unix:path=";
    dbus_session_bus_address += dbus_addr;
    if (setenv("DBUS_SESSION_BUS_ADDRESS", dbus_session_bus_address.c_str(), 1) != 0) {
        log_err("failed to set DBUS_SESSION_BUS_ADDRESS environment variable: {}", dbus_session_bus_address);
        return false;
    }
    return true;
}
