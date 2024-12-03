#pragma once

#include "PulsarCore/Util/Macros.hpp"

#include <memory>
#include <string>

#define SPDLOG_LOG_LEVEL 0
#include "PulsarCore/Result.hpp"

#include <spdlog/spdlog.h>

#define PL_LOG_FATAL(...) SPDLOG_LOGGER_CRITICAL(Pulsar::Log::get_logger(), __VA_ARGS__)
#define PL_LOG_ERROR(...) SPDLOG_LOGGER_ERROR(Pulsar::Log::get_logger(), __VA_ARGS__)
#define PL_LOG_WARN(...) SPDLOG_LOGGER_WARN(Pulsar::Log::get_logger(), __VA_ARGS__)
#define PL_LOG_INFO(...) SPDLOG_LOGGER_INFO(Pulsar::Log::get_logger(), __VA_ARGS__)
#define PL_LOG_DEBUG(...) SPDLOG_LOGGER_DEBUG(Pulsar::Log::get_logger(), __VA_ARGS__)
#define PL_LOG_TRACE(...) SPDLOG_LOGGER_TRACE(Pulsar::Log::get_logger(), __VA_ARGS__)

namespace Pulsar {
    class Log {
    public:
        [[nodiscard]] static Result<bool, std::string> init();
        static void                                    shutdown();

        // NOLINTNEXTLINE(misc-no-recursion)
        [[nodiscard]] static std::shared_ptr<spdlog::logger> get_logger() {
            PULSAR_DEBUG_RUN_ONCE {
                if (s_Logger == nullptr) {
                    if (init().has_value()) {
                        PL_LOG_WARN("Logger was not initialized, using default logger");
                    }
                    else {
                        fmt::println(
                            "Logger was not initialized, and failed to initialize Just-In-Time logger");
                    }
                }
            }
            return s_Logger;
        }

    private:
        inline static std::shared_ptr<spdlog::logger> s_Logger = nullptr;
    };
} // namespace Pulsar
