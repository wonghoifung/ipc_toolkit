#pragma once

#include "base.h"
#include "noncopyable.h"
#include "singleton.h"
#include <functional>
#include <map>
#include <memory>
#include <gio/gio.h>
#include "dbusRegistry.h"
#include "g_dbusConnection.h"

/* glib封装，比libdubs优势是有gloop. */
class G_DbusServer : public G_DbusConnection, noncopyable {
public:
    G_DbusServer(const std::string& address, const std::string& bus);
    ~G_DbusServer();
    void start();
    bool add_interface(const std::string& objName, const std::string& intfName, const DbusInterface& intf);
    inline DbusRegistry& get_registry() {return lut_;}
private:
    DbusRegistry lut_;
};

/* 模拟ubus的简化版, 只需bus-name和方法名/信号名 */
class G_Simplified_DbusServer : public G_DbusServer {
public:
    G_Simplified_DbusServer(const std::string& address, const std::string& bus):G_DbusServer(address,bus) {}
    bool add_interface(const DbusInterface& intf) {
        return G_DbusServer::add_interface(SIMPLIFIED_OBJ,SIMPLIFIED_INTF,intf);
    }
    bool publish_signal(const std::string& signalName, const nlohmann::json& params) {
        return G_DbusConnection::publish_signal(SIMPLIFIED_OBJ,SIMPLIFIED_INTF,signalName,params);
    }
    bool subscribe_signal(const std::string& signalName, dbus_signal_handler cb) {
        return G_DbusConnection::subscribe_signal(SIMPLIFIED_OBJ,SIMPLIFIED_INTF,signalName,cb);
    }
};
