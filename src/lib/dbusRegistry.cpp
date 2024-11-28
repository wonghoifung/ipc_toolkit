#include "dbusRegistry.h"
#include "spdlogger.h"

void dump_registry(DbusRegistry& registry, nlohmann::json& output_json, bool print) {
    for (auto it1 = registry.begin(); it1 != registry.end(); ++it1) {
        const std::string& objName = it1->first;
        DbusObject& obj = it1->second;
        if (print) log_info(">>> object: {}", objName);
        nlohmann::json obj_json;
        for (auto it2 = obj.begin(); it2 != obj.end(); ++it2) {
            const std::string& intfName = it2->first;
            DbusInterface& intf = it2->second;
            DbusMethodHandlers& methodHandlers = intf.first;
            DbusSignalHandlers& signalHandlers = intf.second;
            if (print) log_info(">>> >>> interface: {}", intfName);
            nlohmann::json intf_json;
            nlohmann::json method_json;
            nlohmann::json signal_json;
            for (auto it3 = methodHandlers.begin(); it3 != methodHandlers.end(); ++it3) {
                const std::string& methodName = it3->first;
                if (print) log_info(">>> >>> >>> method: {}", methodName);
                method_json.push_back(methodName);
            }
            for (auto it3 = signalHandlers.begin(); it3 != signalHandlers.end(); ++it3) {
                const std::string& signalName = it3->first;
                if (print) log_info(">>> >>> >>> signal: {}", signalName);
                signal_json.push_back(signalName);
            }
            intf_json["methods"] = method_json;
            intf_json["signals"] = signal_json;
            obj_json[intfName] = intf_json;
        }
        output_json[objName] = obj_json;
    }
}

// https://dbus.freedesktop.org/doc/dbus-specification.html#introspection-format
void dump_object_xml(const std::string& objName, const DbusObject& obj, std::string& output_xml) {
    const char* node_fmt_beg = "<node name=\"%s\">";
    const char* node_fmt_end = "</node>";
    const char* intf_fmt_beg = "<interface name=\"%s\">";
    const char* intf_fmt_end = "</interface>";
    const char* method_fmt_beg = "<method name=\"%s\">";
    const char* method_fmt_end = "</method>";
    const char* method_arg_in = "<arg type='s' name='request' direction='in' />";
    const char* method_arg_out = "<arg type='s' name='response' direction='out' />";
    const char* signal_fmt_beg = "<signal name=\"%s\">";
    const char* signal_fmt_end = "</signal>";
    const char* signal_arg = "<arg name='params' type='s'/>";

    std::stringstream xmlss;
    char buf[1024] = {0};
    snprintf(buf, sizeof buf, node_fmt_beg, objName.c_str());
    xmlss << buf << std::endl;

    for (auto it2 = obj.begin(); it2 != obj.end(); ++it2) {
        const std::string& intfName = it2->first;
        memset(buf, 0, sizeof buf);
        snprintf(buf, sizeof buf, intf_fmt_beg, intfName.c_str());
        xmlss << "\t" << buf << std::endl;
        
        const DbusInterface& intf = it2->second;
        const DbusMethodHandlers& methodHandlers = intf.first;
        const DbusSignalHandlers& signalHandlers = intf.second;
        for (auto it3 = methodHandlers.begin(); it3 != methodHandlers.end(); ++it3) {
            const std::string& methodName = it3->first;
            memset(buf, 0, sizeof buf);
            snprintf(buf, sizeof buf, method_fmt_beg, methodName.c_str());
            xmlss << "\t\t" << buf << std::endl;
            xmlss << "\t\t\t" << method_arg_in << std::endl;
            xmlss << "\t\t\t" << method_arg_out << std::endl;
            xmlss << "\t\t" << method_fmt_end << std::endl;
        }
        for (auto it3 = signalHandlers.begin(); it3 != signalHandlers.end(); ++it3) {
            const std::string& signalName = it3->first;
            memset(buf, 0, sizeof buf);
            snprintf(buf, sizeof buf, signal_fmt_beg, signalName.c_str());
            xmlss << "\t\t" << buf << std::endl;
            xmlss << "\t\t\t" << signal_arg << std::endl;
            xmlss << "\t\t" << signal_fmt_end << std::endl;
        }
        
        xmlss << "\t" << intf_fmt_end << std::endl;
    }

    xmlss << node_fmt_end;

    output_xml = xmlss.str();
}

void dump_all_object_xml(const DbusRegistry& registry) {
    for (auto it1 = registry.begin(); it1 != registry.end(); ++it1) {
        const std::string& objName = it1->first;
        const DbusObject& obj = it1->second;
        std::string xml;
        dump_object_xml(objName, obj, xml);
        log_info("{}: {}", objName, xml);
    }
}

