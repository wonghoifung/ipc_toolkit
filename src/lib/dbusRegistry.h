#pragma once

#include "base.h"
#include <functional>
#include <map>
#include <memory>
#include <vector>

/* 与libdbus和glib无关的通用dbus封装 */

struct DbusReply {
    int code;
    std::string msg;
    nlohmann::json data;
};

typedef std::function<bool(void*, const nlohmann::json&, DbusReply&)> dbus_method_handler;
typedef std::function<void(void*, const nlohmann::json&)> dbus_signal_handler;
typedef std::map<std::string, dbus_method_handler> DbusMethodHandlers;
typedef std::map<std::string, dbus_signal_handler> DbusSignalHandlers;
typedef std::pair<DbusMethodHandlers, DbusSignalHandlers> DbusInterface;
typedef std::map<std::string/*intf-name*/, DbusInterface> DbusObject;
typedef std::map<std::string/*obj-name*/ , DbusObject>    DbusRegistry;

void dump_registry(DbusRegistry& registry, nlohmann::json& output_json, bool print);

// https://dbus.freedesktop.org/doc/dbus-specification.html#introspection-format
void dump_object_xml(const std::string& objName, const DbusObject& obj, std::string& output_xml);
void dump_all_object_xml(const DbusRegistry& registry);

#define SIMPLIFIED_OBJ "/simplified/obj"
#define SIMPLIFIED_INTF "simplified.intf"
