#pragma once

#include "base.h"
#include "noncopyable.h"
#include "singleton.h"
#include "mini.h"

class Config : public toolkit::mINI, noncopyable {
public:
    bool Init(const std::string& fname, const std::string& defaultPath) {
        std::string ini;
        // const char* userdata_dir = std::getenv(USERDATA_ENV_NAME);
        const char* userdata_dir = nullptr;
        std::stringstream ss;
        if (userdata_dir != nullptr) {
            ss << userdata_dir << "/" << fname;
            ini = ss.str();
        } else {
            ss << defaultPath << "/" << fname;
            ini = ss.str();
        }
        if (!fileExists(ini)) {
            dumpFile(ini);
            return true;
        }
        try {
            parseFile(ini);
            return true;
        } catch (std::exception& e) {
            std::cout << "parse ini exception: " << e.what() << std::endl;
            return false;
        }
    }
};
#define G_Config Singleton<Config>::instance()
