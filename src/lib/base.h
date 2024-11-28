#pragma once

#include <chrono>
#include <condition_variable>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <queue>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <list>

#include <nlohmann/json.hpp>

#if defined(SPDLOG_ACTIVE_LEVEL)
#undef SPDLOG_ACTIVE_LEVEL
#endif
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE

#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_sinks.h"
#include "spdlog/spdlog.h"

#define USERDATA_ENV_NAME "USERDATA_DIR"
