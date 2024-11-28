#include "dbusClient.h"
#include "spdlogger.h"
#include "dbusCommon.h"
#include <stdbool.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h> 
#include <string.h>

DbusClient::DbusClient(const std::string& address, const std::string& bus):address_(address),busName_(bus),conn_(nullptr) {

}

DbusClient::~DbusClient() {
   if (conn_) {

   }
}

bool DbusClient::init() {
   if (!set_dbus_addr_env(address_)) return false;

   DBusError err;
   dbus_error_init(&err); 

   conn_ = dbus_bus_get(DBUS_BUS_SESSION, &err);
   if (dbus_error_is_set(&err)) { 
      log_err("dbus connection error {}", err.message); 
      dbus_error_free(&err);
      return false;
   }
   if (nullptr == conn_) { 
      log_err("dbus connection null"); 
      return false;
   } 

   int ret = dbus_bus_request_name(conn_, busName_.c_str(), DBUS_NAME_FLAG_REPLACE_EXISTING , &err);
   if (dbus_error_is_set(&err)) { 
      log_err("dbus name error {}", err.message); 
      dbus_error_free(&err);
      return false;
   }
   if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret) { 
      log_err("dbus not primary owner: {}", ret);
      return false;
   } 
   return true;
}

bool DbusClient::callMethod(const std::string& destBus,
                            const std::string& objName, 
                            const std::string& intfName, 
                            const std::string& methodName, 
                            const nlohmann::json& params,
                            nlohmann::json& reply) {
   DBusMessage* msg = dbus_message_new_method_call(destBus.c_str(), objName.c_str(), intfName.c_str(), methodName.c_str()); 
   if (nullptr == msg) { 
      log_err("dbus message null");
      return false;
   } 

   std::string params_str = params.dump();
   const char* params_cc = params_str.c_str();
   DBusMessageIter args;
   dbus_message_iter_init_append(msg, &args);
   if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &params_cc)) {
      log_err("dbus append msg, maybe out Of memory!"); 
      return false;
   }

   DBusPendingCall* pending;
   if (!dbus_connection_send_with_reply(conn_, msg, &pending, -1)) { // -1 is default timeout
      log_err("dbus send, out Of memory!"); 
      return false;
   }
   if (nullptr == pending) { 
      log_err("dbus pending call null"); 
      return false;
   }
   dbus_connection_flush(conn_);

   dbus_message_unref(msg);

   dbus_pending_call_block(pending); 

   msg = dbus_pending_call_steal_reply(pending);
   if (nullptr == msg) {
      log_err("dbus reply null"); 
      return false;
   }

   dbus_pending_call_unref(pending); 

   if (!dbus_message_iter_init(msg, &args)) {
      log_info("dbus reply message has no arguments"); 
   }
   else if (DBUS_TYPE_STRING != dbus_message_iter_get_arg_type(&args)) {
      log_err("dbus reply argument is not string"); 
   }
   else {
      char* res = nullptr;
      dbus_message_iter_get_basic(&args, &res); 
      if (res && std::string(res) != "") {
         try {
               reply = nlohmann::json::parse(res);
         } catch (const std::exception& e) {
               log_err("dbus argument should be json:{}, exption:{}", res, e.what());
               return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
         }
      } else {
         // log_info("dbus reply empty");
      }
   }

   dbus_message_unref(msg); 
   return true;
}

bool DbusClient::sendSignal(const std::string& objName, 
        const std::string& intfName, 
        const std::string& signalName, 
        const nlohmann::json& params) {
   DBusMessage* msg = dbus_message_new_signal(objName.c_str(), intfName.c_str(), signalName.c_str());
   if (nullptr == msg) { 
      log_err("dbus signal msg null");
      return false;
   } 

   DBusMessageIter args;
   dbus_message_iter_init_append(msg, &args);
   std::string params_str = params.dump();
   const char* params_cc = params_str.c_str();
   if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &params_cc)) {
      log_err("dbus append msg failed, maybe out of memory!");
      return false;
   } 

   dbus_uint32_t serial = 0; 
   if (!dbus_connection_send(conn_, msg, &serial)) {
      log_err("dbus send failed, maybe out of memory!");
      return false;
   }
   dbus_connection_flush(conn_);

   dbus_message_unref(msg);
   return true;
}


