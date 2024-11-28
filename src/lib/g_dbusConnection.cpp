#include "g_dbusConnection.h"
#include "dbusCommon.h"
#include "spdlogger.h"
#include <signal.h>
#include <iostream>
#include <thread>
#include <mutex>

static std::once_flag set_sig_flag;

static void signal_handler(int signum) {
    if (SIGTERM == signum) 
        log_info("dbus disconn, received signal {}, ignore.", signum);
}

static void set_signal_for_dbus() {
    signal(SIGTERM, signal_handler);
}

G_DbusConnection::G_DbusConnection(const std::string& address, const std::string& bus)
    :address_(address),busName_(bus),conn_(nullptr),ownerId_(0),
     bus_acquired_handler_(nullptr),name_acquired_handler_(nullptr),name_lost_handler_(nullptr) {
    std::call_once(set_sig_flag, set_signal_for_dbus);
}

G_DbusConnection::~G_DbusConnection() {
    reconnTimer_.stop();
    fini();
}

bool G_DbusConnection::init() {
    return set_dbus_addr_env(address_);
}

void G_DbusConnection::fini(bool disconn) {
    if (disconn) {
        conn_ = nullptr;
    }
    if (conn_) {
        g_object_unref(conn_);
        conn_ = nullptr;
    }
    if (ownerId_) {
        g_bus_unown_name(ownerId_);
        ownerId_ = 0;
    }
}

static void reconn_timeout(G_Timer* timer) {
    G_DbusConnection* gdconn = (G_DbusConnection*)(timer->get_user_data());
    if (gdconn == nullptr) {
        log_err("G_DbusConnection* is null");
    } else {
        bool res = gdconn->own_name(
            gdconn->get_bus_acquired_handler(), 
            gdconn->get_name_acquired_handler(), 
            gdconn->get_name_lost_handler());
        log_info("reconn res: {}", res);
    }
}

void G_DbusConnection::set_reconn_timer() {
    reconnTimer_.start_once(1000, reconn_timeout, this);
}

static void on_bus_acquired(GDBusConnection* connection, const gchar* name, gpointer user_data) {
    log_info("on_bus_acquired: {}", name);
    G_DbusConnection* gdconn = (G_DbusConnection*)user_data;
    if (!gdconn) {
        log_err("not found G_DbusConnection* for {}", name);
        return;
    }
    gdconn->set_bus_conn(connection);
    auto cb = gdconn->get_bus_acquired_handler();
    if (cb) {
        cb(connection, name, user_data);
    }
}

static void on_name_acquired(GDBusConnection* connection, const gchar* name, gpointer user_data) {
    log_info("on_name_acquired: {}", name);
    G_DbusConnection* gdconn = (G_DbusConnection*)user_data;
    if (!gdconn) {
        log_err("not found G_DbusConnection* for {}", name);
        return;
    }
    auto cb = gdconn->get_name_acquired_handler();
    if (cb) {
        cb(connection, name, user_data);
    }
}

static void on_name_lost(GDBusConnection* connection, const gchar* name, gpointer user_data) {  
    log_info("on_name_lost: {}, connection is null: {}", name, (connection == nullptr));
    G_DbusConnection* gdconn = (G_DbusConnection*)user_data;
    if (!gdconn) {
        log_err("not found G_DbusConnection* for {}", name);
        return;
    }
    auto cb = gdconn->get_name_lost_handler();
    if (cb) {
        cb(connection, name, user_data);
    }
    gdconn->fini(true);
    gdconn->set_reconn_timer();
} 

bool G_DbusConnection::own_name(GBusAcquiredCallback bus_acquired_handler,
                                GBusNameAcquiredCallback name_acquired_handler,
                                GBusNameLostCallback name_lost_handler) {
    bus_acquired_handler_ = bus_acquired_handler;
    name_acquired_handler_ = name_acquired_handler;
    name_lost_handler_ = name_lost_handler;
    GBusNameOwnerFlags flags = (GBusNameOwnerFlags)(G_BUS_NAME_OWNER_FLAGS_ALLOW_REPLACEMENT | G_BUS_NAME_OWNER_FLAGS_REPLACE);
    // 需要用g_bus_unown_name释放, 见fini()
    ownerId_ = g_bus_own_name(G_BUS_TYPE_SESSION,
                            busName_.c_str(),
                            flags,
                            on_bus_acquired,
                            on_name_acquired,
                            on_name_lost,
                            this,NULL);
    log_info("glib dbus owner-id: {}", ownerId_);
    return true;
}

bool G_DbusConnection::publish_signal(const std::string& objName, 
                                const std::string& intfName, 
                                const std::string& signalName, 
                                const std::string& paramsStr) {
    if (!get_bus_ready()) {
        log_err("bus not ready, cannot publish");
        return false;
    }
    
    // g_variant_new需要用g_variant_unref释放
    // 但signal_parameters在后面被g_dbus_connection_emit_signal通过g_dbus_message_set_body拿走ownership
    // 所以这个signal_parameters不需要我负责释放
    GVariant* signal_parameters = g_variant_new("(s)", paramsStr.c_str());
    GError* error = NULL;
    g_dbus_connection_emit_signal(conn_,
                                  NULL,
                                  objName.c_str(),
                                  intfName.c_str(), 
                                  signalName.c_str(), 
                                  signal_parameters, 
                                  &error); 
    if (error != NULL) {
        log_err("failed to g_dbus_connection_emit_signal: {}", error->message);
        g_error_free(error);
        return false;
    }
    return true;
}

bool G_DbusConnection::publish_signal(const std::string& objName, 
                                const std::string& intfName, 
                                const std::string& signalName, 
                                const nlohmann::json& params) {
    std::string paramsStr = params.dump();
    return G_DbusConnection::publish_signal(objName,intfName,signalName,paramsStr);
}

static void dispatch_signal(GDBusConnection* connection,
                        const gchar* sender_name,
                        const gchar* object_path,
                        const gchar* interface_name,
                        const gchar* signal_name,
                        GVariant* parameters,
                        gpointer user_data) {
    G_DbusConnection* gconn = (G_DbusConnection*)user_data;
    if (gconn == nullptr) {
        log_err("G_DbusConnection* is null");
        return;
    }
    dbus_signal_handler cb = gconn->get_signal_cb(object_path, interface_name, signal_name);
    if (cb) {
        // inputvv需要释放, input不用释放
        GVariant* inputvv = g_variant_get_child_value(parameters, 0);
        const gchar* input = g_variant_get_string(inputvv, NULL);  
        nlohmann::json inputJson;
        if (input && g_strcmp0(input, "")) {
            try {
                inputJson = nlohmann::json::parse(input);
            } catch (const std::exception& e) {
                log_err("dbus argument should be json:{}, exption:{}", input, e.what());
                if (inputvv) g_variant_unref(inputvv);
                return;
            }
        }
        cb(user_data, inputJson);
        if (inputvv) g_variant_unref(inputvv);
    }
}

static std::string gen_signal_key(const std::string& objName, 
                                const std::string& intfName, 
                                const std::string& signalName) {
    std::stringstream keyss;
    keyss << objName << "-" << intfName << "-" << signalName;
    return keyss.str();
}

bool G_DbusConnection::subscribe_signal(const std::string& objName, 
                                const std::string& intfName, 
                                const std::string& signalName,
                                dbus_signal_handler cb) {
    if (!get_bus_ready()) {
        log_err("bus not ready, cannot subscribe");
        return false;
    }

    std::string key = gen_signal_key(objName, intfName, signalName);
    signal_cb_lut_[key] = cb;

    guint subid = g_dbus_connection_signal_subscribe(conn_,
                                                    NULL,
                                                    intfName.c_str(),
                                                    signalName.c_str(),
                                                    objName.c_str(),
                                                    NULL,
                                                    G_DBUS_SIGNAL_FLAGS_NONE,
                                                    dispatch_signal,
                                                    this,
                                                    NULL);
    log_info("obj:{} intf:{} sig:{} sub-id:{}", objName, intfName, signalName, subid);
    return true;
}

dbus_signal_handler G_DbusConnection::get_signal_cb(const std::string& objName, 
                                            const std::string& intfName, 
                                            const std::string& signalName) {
    std::string key = gen_signal_key(objName, intfName, signalName);
    auto it = signal_cb_lut_.find(key);
    if (it != signal_cb_lut_.end()) return it->second;
    return nullptr;
}
