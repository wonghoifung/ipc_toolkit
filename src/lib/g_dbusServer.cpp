#include "g_dbusServer.h"
#include "dbusCommon.h"
#include "spdlogger.h"

G_DbusServer::G_DbusServer(const std::string& address, const std::string& bus):G_DbusConnection(address,bus) {

}

G_DbusServer::~G_DbusServer() {

}

static void simple_method_reply(GDBusMethodInvocation* invocation, int code, const std::string& msg, nlohmann::json* data = nullptr) {
    nlohmann::json response;
    response["code"] = code;
    response["msg"] = msg;
    if (data) {
        response["data"] = *data;
    }
    std::string reply = response.dump();
    // g_dbus_method_invocation_return_value调用的g_variant_new不用自己释放
    g_dbus_method_invocation_return_value(invocation, g_variant_new("(s)", reply.c_str()));
}

static void dispatch_method_call(GDBusConnection* connection,  
                                const gchar* sender,  
                                const gchar* object_path,  
                                const gchar* interface_name,  
                                const gchar* method_name,  
                                GVariant* parameters,  
                                GDBusMethodInvocation* invocation,  
                                gpointer user_data) {  

    G_DbusServer* server = (G_DbusServer*)user_data;
    DbusRegistry& registry = server->get_registry();

    auto obj_it = registry.find(object_path);
    if (registry.end() == obj_it) {
        simple_method_reply(invocation, 1, "unknow object");
        return;
    }
    DbusObject& object = obj_it->second;

    auto intf_it = object.find(interface_name);
    if (object.end() == intf_it) {
        simple_method_reply(invocation, 2, "unknow interface");
        return;
    }
    DbusInterface& interface = intf_it->second;
    DbusMethodHandlers& methodHandlers = interface.first;

    auto method_it = methodHandlers.find(method_name);
    if (methodHandlers.end() == method_it) {
        simple_method_reply(invocation, 3, "unknow method");
        return;
    }
    dbus_method_handler& method_cb = method_it->second;

    // inputvv需要被自己释放
    GVariant* inputvv = g_variant_get_child_value(parameters, 0);
    const gchar* input = g_variant_get_string(inputvv, NULL);  
    nlohmann::json inputJson;
    if (input && g_strcmp0(input, "")) {
        try {
            inputJson = nlohmann::json::parse(input);
        } catch (const std::exception& e) {
            log_err("dbus argument should be json:{}, exption:{}", input, e.what());
            simple_method_reply(invocation, 4, "unknow json param");
            if (inputvv) g_variant_unref(inputvv);
            return;
        }
    }

    DbusReply output;
    bool ok = method_cb(server, inputJson, output);

    if (ok) {
        simple_method_reply(invocation, output.code, output.msg, &(output.data));
    } else {
        simple_method_reply(invocation, -1, "callback return failed");
    }

    if (inputvv) g_variant_unref(inputvv);
}

// https://docs.gtk.org/gio/method.DBusNodeInfo.lookup_interface.html
static void on_bus_acquired(GDBusConnection* connection, const gchar* name, gpointer user_data) {
    log_info("server on_bus_acquired: {}", name);

    G_DbusServer* server = (G_DbusServer*)user_data;
    DbusRegistry& registry = server->get_registry();

    GDBusNodeInfo* introspection_data;
    GDBusInterfaceInfo* interface_info;
    GDBusInterfaceVTable interface_vtable = {
        .method_call = dispatch_method_call,
    };

    GError* error = NULL;

    for (auto it1 = registry.begin(); it1 != registry.end(); ++it1) {
        const std::string& objName = it1->first;
        const DbusObject& obj = it1->second;
        std::string xml;
        dump_object_xml(objName, obj, xml);
        
        gchar* introspection_xml = (gchar*)(xml.c_str());
        // 需要自己释放introspection_data
        introspection_data = g_dbus_node_info_new_for_xml(introspection_xml, &error);
        if (error != NULL) {
            log_err("failed to create introspection data: {}", error->message);
            if (introspection_data) g_dbus_node_info_unref(introspection_data);
            return;
        }

        for (auto it2 = obj.begin(); it2 != obj.end(); ++it2) {
            const std::string& intfName = it2->first;
            // 不用释放interface_info
            interface_info = g_dbus_node_info_lookup_interface(introspection_data,intfName.c_str());
            if (interface_info == NULL) {
                log_err("failed to lookup interface info: {}", intfName);
                g_dbus_node_info_unref(introspection_data);
                return;
            }
            // 反注册g_dbus_connection_unregister_object
            guint registration_id = g_dbus_connection_register_object(connection,objName.c_str(),interface_info,&interface_vtable,user_data,NULL,&error);
            if (error != NULL) {
                log_err("g_dbus_connection_register_object: {}", error->message);
                g_dbus_node_info_unref(introspection_data);
                return;
            }
            if(registration_id == 0) {
                log_err("failed to register object:{} intf:{}", objName, intfName);
                g_dbus_node_info_unref(introspection_data);
                return;   
            }
            log_info("obj:{} intf:{} registration-id:{}", objName, intfName, registration_id);
            // g_dbus_connection_unregister_object(registration_id); 

            const DbusInterface& intf = it2->second;
            const DbusSignalHandlers& signalHandlers = intf.second;
            for (auto it3 = signalHandlers.begin(); it3 != signalHandlers.end(); ++it3) {
                const std::string& signalName = it3->first;
                auto cb = it3->second;
                server->subscribe_signal(objName, intfName, signalName, cb);
            }
        }
        g_dbus_node_info_unref(introspection_data);
    }
}

static void on_name_acquired(GDBusConnection* connection, const gchar* name, gpointer user_data) {
    log_info("server on_name_acquired: {}", name);
}

static void on_name_lost(GDBusConnection* connection, const gchar* name, gpointer user_data) {  
    log_info("server on_name_lost: {}, connection is null: {}", name, (connection == nullptr));
} 

void G_DbusServer::start() {
    own_name(on_bus_acquired,on_name_acquired,on_name_lost);
}

bool G_DbusServer::add_interface(const std::string& objName, const std::string& intfName, const DbusInterface& intf) {
    lut_[objName][intfName] = intf;
    return true;
}
