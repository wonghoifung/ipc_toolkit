#pragma once

#include "base.h"
#include "noncopyable.h"
#include "singleton.h"
#include <dbus/dbus.h>
#include "dbusRegistry.h"

/* 这个类使用了最底层的libdbus库 https://dbus.freedesktop.org/doc/api/html/, 
   但是我后面发现用glib的话有一个gloop可以用, 
   所以建议使用g_dbusServer.h提供的glib封装版本.
   据说libdubs能提供更细节的控制, 所以保留, 不用就行. */
class DbusServer : noncopyable {
public:
    DbusServer(const std::string& address, const std::string& bus);
    ~DbusServer();
    bool init();
    void run();
    void dump();
    bool add_interface(const std::string& objName, const std::string& intfName, const DbusInterface& intf);
    inline DbusRegistry& get_registry() {return lut_;}
private:
    void set_intro_path();
    void set_intro_method();
    friend bool Introspect(void* server, const nlohmann::json& input, DbusReply& reply);
private:
    std::string address_;
    std::string busName_;
    DBusConnection* conn_;
    DbusRegistry lut_;
    std::string introspect_path_;
};

