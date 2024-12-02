#include "Log.hpp"

#include <spdlog/sinks/stdout_color_sinks.h>

namespace Pulsar {
    Result<bool, std::string> Log::Init() {
        s_Logger = spdlog::stdout_color_mt("Pulsar");
        s_Logger->set_level(spdlog::level::trace);
        s_Logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");

        if (s_Logger == nullptr) {
            return Err<std::string>("Failed to initialize logger");
        }
        PL_LOG_DEBUG("Logger initialized");
        return Result<bool, std::string>(true);
    }

    void Log::Shutdown() {
        s_Logger = nullptr;
        spdlog::shutdown();
    }
} // namespace Pulsar
