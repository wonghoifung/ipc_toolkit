#pragma once

#include "base.h"
#include "noncopyable.h"
#include "singleton.h"
#include <memory>
#include <iostream>

class SpdLogger : noncopyable {
public:
    std::shared_ptr<spdlog::logger>& getSpdLogger()
    {
        return spd_logger;
    }

    SpdLogger(): max_file_num(0), max_file_size_mb(10) {
        
    }

    ~SpdLogger() {

    }

    void Init(const std::string& fname, const std::string& defaultPath) {
        const char* userdata_dir = std::getenv(USERDATA_ENV_NAME);
        std::stringstream ss;
        if (userdata_dir != nullptr) {
            ss << userdata_dir << "/" << fname;
            log_file = ss.str();
        } else {
            ss << defaultPath << "/" << fname;
            log_file = ss.str();
        }
        auto rotating_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(log_file, max_file_size_mb * 1024 * 1024, max_file_num);
        auto console_sink = std::make_shared<spdlog::sinks::stdout_sink_mt>();
        spd_logger = std::make_shared<spdlog::logger>("logger", spdlog::sinks_init_list { console_sink, rotating_sink });
        spd_logger->set_level(spdlog::level::debug);
        spd_logger->set_pattern("[%l] \"%Y-%m-%d %T\" (%s:%#) - %v");
        spd_logger->flush_on(spdlog::level::trace);
    }

private:
    std::string log_file;
    int max_file_num;
    int max_file_size_mb;
    std::shared_ptr<spdlog::logger> spd_logger;
};
#define G_Logger Singleton<SpdLogger>::instance()
#define log_debug(format, ...) SPDLOG_LOGGER_DEBUG(Singleton<SpdLogger>::instance().getSpdLogger(), format, ##__VA_ARGS__)
#define log_info(format, ...) SPDLOG_LOGGER_INFO(Singleton<SpdLogger>::instance().getSpdLogger(), format, ##__VA_ARGS__)
#define log_warn(format, ...) SPDLOG_LOGGER_WARN(Singleton<SpdLogger>::instance().getSpdLogger(), format, ##__VA_ARGS__)
#define log_err(format, ...) SPDLOG_LOGGER_ERROR(Singleton<SpdLogger>::instance().getSpdLogger(), format, ##__VA_ARGS__)

