#pragma once

#include "base.h"
#include <gio/gio.h>
#include "dbusRegistry.h"
#include "g_timer.h"

/* glib封装，比libdubs优势是有gloop.
   https://docs.gtk.org/gio/ 
   https://docs.gtk.org/glib */
   
class G_DbusConnection {
public:
    G_DbusConnection(const std::string& address, const std::string& bus);
    virtual ~G_DbusConnection();
    bool init();
    void fini(bool disconn = false);
    void set_reconn_timer();
    bool own_name(GBusAcquiredCallback bus_acquired_handler,
                  GBusNameAcquiredCallback name_acquired_handler,
                  GBusNameLostCallback name_lost_handler);

    inline const bool get_bus_ready() const {return conn_ != nullptr;}
    inline GBusAcquiredCallback get_bus_acquired_handler() {return bus_acquired_handler_;}
    inline GBusNameAcquiredCallback get_name_acquired_handler() {return name_acquired_handler_;}
    inline GBusNameLostCallback get_name_lost_handler() {return name_lost_handler_;}
    inline void set_bus_conn(GDBusConnection* conn) {conn_=conn;}

    bool publish_signal(const std::string& objName, 
        const std::string& intfName, 
        const std::string& signalName, 
        const std::string& paramsStr);
    bool publish_signal(const std::string& objName, 
        const std::string& intfName, 
        const std::string& signalName, 
        const nlohmann::json& params);
    bool subscribe_signal(const std::string& objName, 
        const std::string& intfName, 
        const std::string& signalName,
        dbus_signal_handler cb);
    dbus_signal_handler get_signal_cb(const std::string& objName, 
                            const std::string& intfName, 
                            const std::string& signalName);

protected:
    std::string address_;
    std::string busName_;
    GDBusConnection* conn_;
    guint ownerId_;
    GBusAcquiredCallback bus_acquired_handler_;
    GBusNameAcquiredCallback name_acquired_handler_;
    GBusNameLostCallback name_lost_handler_;
    std::map<std::string, dbus_signal_handler> signal_cb_lut_;
    G_Timer reconnTimer_;
};

