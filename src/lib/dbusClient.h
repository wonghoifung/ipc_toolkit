#pragma once

#include "base.h"
#include "noncopyable.h"
#include "singleton.h"
#include <dbus/dbus.h>

/* 这个类使用了最底层的libdbus库 https://dbus.freedesktop.org/doc/api/html/, 
   但是我后面发现用glib的话有一个gloop可以用, 
   所以建议使用g_dbusClient.h提供的glib封装版本.
   据说libdubs能提供更细节的控制, 所以保留, 不用就行. */
class DbusClient : noncopyable {
public:
    DbusClient(const std::string& address, const std::string& bus);
    ~DbusClient();
    bool init();
    bool callMethod(const std::string& destBus,
        const std::string& objName, 
        const std::string& intfName, 
        const std::string& methodName, 
        const nlohmann::json& params,
        nlohmann::json& reply);
    bool sendSignal(const std::string& objName, 
        const std::string& intfName, 
        const std::string& signalName, 
        const nlohmann::json& params);
private:
    std::string address_;
    std::string busName_;
    DBusConnection* conn_;
};

