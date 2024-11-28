#include "dbusServer.h"
#include "spdlogger.h"
#include "dbusCommon.h"
#include <stdbool.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h> 
#include <string.h>

static DBusHandlerResult dbus_message_dispatch(DBusConnection* connection, DBusMessage* message, void* user_data) {
    DbusServer* server = (DbusServer*)user_data;
    DbusRegistry& registry = server->get_registry();

    const char* path = dbus_message_get_path(message);
    if (nullptr == path){
        log_err("null path message");
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

    const char* sender = dbus_message_get_sender(message);
    const char* dest = dbus_message_get_destination(message);

    auto obj_it = registry.find(path);
    if (registry.end() == obj_it) {
        if (std::string("/org/freedesktop/DBus") == path) {        
            const char* errname = dbus_message_get_error_name(message);
            const char* signature = dbus_message_get_signature(message);
            const char* intf = dbus_message_get_interface(message);
            const char* member = dbus_message_get_member(message);
            log_info("FROM SYSTEM PATH:{}, SENDER:{}, DEST:{}, ERR:{}, SIGNATURE:{}, INTF:{}, MEMBER:{}",
                path == nullptr ? "" : path, 
                sender == nullptr ? "" : sender, 
                dest == nullptr ? "" : dest, 
                errname == nullptr ? "" : errname, 
                signature == nullptr ? "" : signature, 
                intf == nullptr ? "" : intf, 
                member == nullptr ? "" : member);
            return DBUS_HANDLER_RESULT_HANDLED;
        }
        log_err("not found object path: {}", path);
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }
    DbusObject& object = obj_it->second;

    const char* intf = dbus_message_get_interface(message);
    if (nullptr == intf) {
        log_err("null interface message");
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

    auto intf_it = object.find(intf);
    if (object.end() == intf_it) {
        log_err("not found interface: {}", intf);
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }
    DbusInterface& interface = intf_it->second;
    DbusMethodHandlers& methodHandlers = interface.first;
    DbusSignalHandlers& signalHandlers = interface.second;

    const char* member = dbus_message_get_member(message);
    if (nullptr == member) {
        log_err("null member message");
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

    log_info("FROM SESSION PATH:{}, SENDER:{}, DEST:{}, INTF:{}, MEMBER:{}",
        path == nullptr ? "" : path, 
        sender == nullptr ? "" : sender, 
        dest == nullptr ? "" : dest,
        intf == nullptr ? "" : intf, 
        member == nullptr ? "" : member);

    bool isSignal = false;
    if (dbus_message_is_signal(message, intf, member)) {
        isSignal = true;
    }

    nlohmann::json input;
    DBusMessageIter args;
    if (!dbus_message_iter_init(message, &args)) {
        // no argument, do nothing.
    }
    else if (DBUS_TYPE_STRING != dbus_message_iter_get_arg_type(&args)) {
        log_err("dbus argument should be string");
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }
    else { 
        char* param;
        dbus_message_iter_get_basic(&args, &param);
        if (param && std::string(param) != "") {
            try {
                input = nlohmann::json::parse(param);
            } catch (const std::exception& e) {
                log_err("dbus argument should be json:{}, exption:{}", param, e.what());
                return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
            }
        }
    }

    if (isSignal) {
        auto signal_it = signalHandlers.find(member);
        if (signalHandlers.end() == signal_it) {
            log_err("signal-handlers not found callback: {}", member);
            return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
        }
        dbus_signal_handler& signal_cb = signal_it->second;
        signal_cb(server, input);
        return DBUS_HANDLER_RESULT_HANDLED;
    }

    auto method_it = methodHandlers.find(member);
    if (methodHandlers.end() == method_it) {
        log_err("method-handlers not found callback: {}", member);
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }
    dbus_method_handler& method_cb = method_it->second;

    DbusReply output;
    bool ok = method_cb(server, input, output);

    DBusMessage* reply = dbus_message_new_method_return(message); 
    if (reply == nullptr) {
        log_err("cannot allocate reply");
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

    nlohmann::json output_json;
    std::string str_output_json;
    if (ok) {
        output_json["code"] = output.code;
        output_json["msg"] = output.msg;
        output_json["data"] = output.data;
        str_output_json = output_json.dump();
    } else {
        nlohmann::json output_json;
        output_json["code"] = -1;
        output_json["msg"] = "callback return failed";
        str_output_json = output_json.dump();
    }
    const char* c_output_json = str_output_json.c_str();
    dbus_message_append_args(reply, DBUS_TYPE_STRING, &c_output_json, DBUS_TYPE_INVALID);
    if (!dbus_connection_send(connection, reply, nullptr)) {
        log_err("dbus reply failed");
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }
    dbus_connection_flush(connection);
    dbus_message_unref(reply);
    return DBUS_HANDLER_RESULT_HANDLED;
}

DbusServer::DbusServer(const std::string& address, const std::string& bus):address_(address),busName_(bus),conn_(nullptr) {
    set_intro_path();
}

DbusServer::~DbusServer() {
    if (conn_) {
        
    }
}

bool DbusServer::init() {
    if (!set_dbus_addr_env(address_)) return false;

    DBusError err;
    dbus_error_init(&err);

    conn_ = dbus_bus_get(DBUS_BUS_SESSION, &err);
    if (dbus_error_is_set(&err)) { 
        log_err("dbus connection error: {}", err.message); 
        dbus_error_free(&err); 
        return false;
    }
    if (nullptr == conn_) {
        log_err("dbus connection null"); 
        return false;
    }

    int ret = dbus_bus_request_name(conn_, busName_.c_str(), DBUS_NAME_FLAG_REPLACE_EXISTING , &err);
    if (dbus_error_is_set(&err)) { 
        log_err("dbus name error: {}", err.message); 
        dbus_error_free(&err);
        return false;
    }
    if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret) { 
        log_err("dbus not primary owner: {}", ret);
        return false;
    } 

    if (!dbus_connection_add_filter(conn_, dbus_message_dispatch, this, nullptr)) {
        log_err("dbus add filter error");
        return false;
    } 

    set_intro_method();
    return true;
}

void DbusServer::run() {
    log_info("dbus listen start, address:{}, busName:{}", address_, busName_); 
    dump();
    while (dbus_connection_read_write_dispatch(conn_, -1)) {
        sleep(0);
    }
}

void DbusServer::dump() {
    nlohmann::json output_json;
    dump_registry(lut_, output_json, true);
    std::ofstream f(introspect_path_);
    f << output_json.dump() << std::endl;
    f.flush();
    f.close();
    dump_all_object_xml(lut_);
}

bool DbusServer::add_interface(const std::string& objName, const std::string& intfName, const DbusInterface& intf) {
    auto obj_it = lut_.find(objName);
    if (obj_it != lut_.end()) {
        auto intf_it = obj_it->second.find(intfName);
        if (intf_it != obj_it->second.end()) {
            DbusInterface& currentIntf = intf_it->second;
            DbusMethodHandlers& current_methodHandlers = currentIntf.first;
            DbusSignalHandlers& current_signalHandlers = currentIntf.second;
            const DbusMethodHandlers& new_methodHandlers = intf.first;
            const DbusSignalHandlers& new_signalHandlers = intf.second;
            for (const auto& pair : new_methodHandlers) {
                current_methodHandlers.insert(pair);
            }
            for (const auto& pair : new_signalHandlers) {
                current_signalHandlers.insert(pair);
            }
            log_info("{} {} added. no need to add_match", objName, intfName);
            return true;
        }
    }

    lut_[objName][intfName] = intf;

    DBusError err;
    dbus_error_init(&err);

    std::stringstream matchss;
    matchss << "path='" << objName << "',interface='" << intfName << "'";
    std::string matchs = matchss.str();

    std::string type_method_call("type='method_call',");
    type_method_call += matchs;
    std::string type_signal("type='signal',");
    type_signal += matchs;

    log_info("type_method_call: {}", type_method_call);
    dbus_bus_add_match(conn_, type_method_call.c_str(), &err);
    dbus_connection_flush(conn_);
    if (dbus_error_is_set(&err)) { 
        log_err("dbus method match error {}", err.message);
        return false;
    }

    log_info("type_signal: {}", type_signal);
    dbus_bus_add_match(conn_, type_signal.c_str(), &err);
    dbus_connection_flush(conn_);
    if (dbus_error_is_set(&err)) { 
        log_err("dbus signal match error {}", err.message);
        return false;
    }

    log_info("{} {} added. also add_match", objName, intfName);
    return true;
}

void DbusServer::set_intro_path() {
    std::stringstream ss;
    ss << "/tmp/introspect_" << busName_ << ".json";
    introspect_path_ = ss.str();
}

bool Introspect(void* server, const nlohmann::json& input, DbusReply& reply) {
    DbusServer* ds = (DbusServer*)server;
    reply.code = 0;
    reply.msg = "success";
    reply.data["address"] = ds->address_;
    reply.data["busName"] = ds->busName_;
    nlohmann::json output_json;
    dump_registry(ds->lut_, output_json, false);
    reply.data["service"] = output_json;
    return true;
}

void DbusServer::set_intro_method() {
    DbusInterface intf;
    DbusMethodHandlers& methodHandlers = intf.first;
    methodHandlers["Introspect"] = Introspect;
    add_interface("/ipc/toolkit","ipc.toolkit.DBus.Introspectable",intf);
}
