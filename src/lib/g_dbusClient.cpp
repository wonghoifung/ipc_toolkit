#include "g_dbusClient.h"
#include "spdlogger.h"

G_DbusClient::G_DbusClient(const std::string& address, const std::string& bus):G_DbusConnection(address,bus) {

}

G_DbusClient::~G_DbusClient() {
    clear_proxy_cache();
}

static void on_bus_acquired(GDBusConnection* connection, const gchar* name, gpointer user_data) {
    log_info("client on_bus_acquired: {}", name);
    G_DbusClient* client = (G_DbusClient*)user_data;
    auto cb = client->get_bus_ready_cb();
    cb(client);
}

static void on_name_acquired(GDBusConnection* connection, const gchar* name, gpointer user_data) {
    log_info("client on_name_acquired: {}", name);
}

static void on_name_lost(GDBusConnection* connection, const gchar* name, gpointer user_data) {  
    log_info("client on_name_lost: {}, connection is null: {}", name, (connection == nullptr));
    G_DbusClient* client = (G_DbusClient*)user_data;
    if (client) client->clear_proxy_cache();
} 

bool G_DbusClient::init(bus_ready_handler ready_cb) {
    if (!G_DbusConnection::init()) return false;
    own_name(on_bus_acquired,on_name_acquired,on_name_lost);
    bus_ready_cb_ = ready_cb;
    return true;
}

bool G_DbusClient::call_method(const std::string& destBus,
                            const std::string& objName, 
                            const std::string& intfName, 
                            const std::string& methodName, 
                            const std::string& paramsStr,
                            nlohmann::json& reply) {
    if (!get_bus_ready()) {
        log_err("bus not ready, cannot call method");
        return false;
    }

    GDBusProxy* proxy = get_set_proxy(destBus, objName, intfName);
    if (!proxy) return false;

    GError* error = NULL;
    // g_dbus_proxy_call_sync里调用的g_variant_new不用自己释放, result需要释放
    GVariant* result = g_dbus_proxy_call_sync(proxy,
                                              methodName.c_str(),
                                              g_variant_new("(s)", paramsStr.c_str()),
                                              G_DBUS_CALL_FLAGS_NONE,
                                              -1,
                                              NULL,
                                              &error);
    if (error != NULL) {
        log_err("failed to call remote method: {}", error->message);
        g_error_free(error);
        return false;
    }

    gchar* response;
    g_variant_get(result, "(s)", &response);
    if (response && g_strcmp0(response, "")) {
        try {
            reply = nlohmann::json::parse(response);
        } catch (const std::exception& e) {
            log_err("dbus argument should be json:{}, exption:{}", response, e.what());
            if (result) g_variant_unref(result);
            return false;
        }
    }
    if (result) g_variant_unref(result);
    return true;
}

bool G_DbusClient::call_method(const std::string& destBus,
                            const std::string& objName, 
                            const std::string& intfName, 
                            const std::string& methodName, 
                            const nlohmann::json& params,
                            nlohmann::json& reply) {

    std::string paramsStr = params.dump();
    return G_DbusClient::call_method(destBus,objName,intfName,methodName,paramsStr,reply);
}

void G_DbusClient::clear_proxy_cache() {
    for (auto it = proxy_cache_.begin(); it != proxy_cache_.end(); ++it) {
        if (it->second) {
            g_object_unref(it->second);
        }
    }
    proxy_cache_.clear();
}

GDBusProxy* G_DbusClient::get_set_proxy(const std::string& busName,
                            const std::string& objName, 
                            const std::string& intfName) {
    std::stringstream keyss;
    keyss << busName << "-" << objName << "-" << intfName;
    std::string key = keyss.str();

    auto it = proxy_cache_.find(key);
    if (it != proxy_cache_.end()) {
        return it->second;
    }

    GError* error = NULL;
    // proxy需要释放, 见clear_proxy_cache()
    GDBusProxy* proxy = g_dbus_proxy_new_sync(conn_,
                                              G_DBUS_PROXY_FLAGS_NONE,
                                              NULL,
                                              busName.c_str(),
                                              objName.c_str(),
                                              intfName.c_str(),
                                              NULL,
                                              &error);
    if (error != NULL) {
        log_err("failed to create proxy: {}", error->message);
        g_error_free(error);
        return nullptr;
    }

    proxy_cache_[key] = proxy;
    return proxy;
}



