#pragma once

#include "base.h"
#include "noncopyable.h"
#include <gio/gio.h>
#include "g_dbusConnection.h"
#include "dbusRegistry.h"
#include <functional>

typedef std::function<void(void*)> bus_ready_handler;

/* glib封装，比libdubs优势是有gloop. */
class G_DbusClient : public G_DbusConnection, noncopyable {
public:
    G_DbusClient(const std::string& address, const std::string& bus);
    ~G_DbusClient();
    bool init(bus_ready_handler ready_cb);
    bool call_method(const std::string& destBus,
        const std::string& objName, 
        const std::string& intfName, 
        const std::string& methodName, 
        const std::string& paramsStr,
        nlohmann::json& reply);
    bool call_method(const std::string& destBus,
        const std::string& objName, 
        const std::string& intfName, 
        const std::string& methodName, 
        const nlohmann::json& params,
        nlohmann::json& reply);
    void clear_proxy_cache();
private:
    GDBusProxy* get_set_proxy(const std::string& busName,
                            const std::string& objName, 
                            const std::string& intfName);
public:
    bus_ready_handler get_bus_ready_cb() {return bus_ready_cb_;}
    GDBusConnection* get_conn() {return conn_;}
private:
    std::map<std::string, GDBusProxy*> proxy_cache_;
    bus_ready_handler bus_ready_cb_;
};

/* 模拟ubus的简化版, 只需bus-name和方法名/信号名 */
class G_Simplified_DbusClient : public G_DbusClient {
public:
    G_Simplified_DbusClient(const std::string& address, const std::string& bus):G_DbusClient(address,bus) {}
    bool call_method(const std::string& destBus,
        const std::string& methodName, 
        const nlohmann::json& params,
        nlohmann::json& reply) {
        return G_DbusClient::call_method(destBus,SIMPLIFIED_OBJ,SIMPLIFIED_INTF,methodName,params,reply);
    }
    bool call_method(const std::string& destBus,
        const std::string& methodName, 
        const std::string& paramsStr,
        nlohmann::json& reply) {
        return G_DbusClient::call_method(destBus,SIMPLIFIED_OBJ,SIMPLIFIED_INTF,methodName,paramsStr,reply);
    }
    bool publish_signal(const std::string& signalName, const nlohmann::json& params) {
        return G_DbusConnection::publish_signal(SIMPLIFIED_OBJ,SIMPLIFIED_INTF,signalName,params);
    }
    bool subscribe_signal(const std::string& signalName, dbus_signal_handler cb) {
        return G_DbusConnection::subscribe_signal(SIMPLIFIED_OBJ,SIMPLIFIED_INTF,signalName,cb);
    }
};
