#pragma once

#include <memory>
#include <string>

#define SPDLOG_LOG_LEVEL 0
#include <spdlog/spdlog.h>

#include "PulsarCore/Result.hpp"

namespace Pulsar {
    class Log {
    public:
        [[nodiscard]] static Result<bool, std::string> init();
        static void                                    shutdown();

        [[nodiscard]] static std::shared_ptr<spdlog::logger> get_logger() {
            return s_Logger;
        }

    private:
        inline static std::shared_ptr<spdlog::logger> s_Logger = nullptr;
    };
} // namespace Pulsar

#define PL_LOG_FATAL(...) SPDLOG_LOGGER_CRITICAL(Pulsar::Log::get_logger(), __VA_ARGS__)
#define PL_LOG_ERROR(...) SPDLOG_LOGGER_ERROR(Pulsar::Log::get_logger(), __VA_ARGS__)
#define PL_LOG_WARN(...) SPDLOG_LOGGER_WARN(Pulsar::Log::get_logger(), __VA_ARGS__)
#define PL_LOG_INFO(...) SPDLOG_LOGGER_INFO(Pulsar::Log::get_logger(), __VA_ARGS__)
#define PL_LOG_DEBUG(...) SPDLOG_LOGGER_DEBUG(Pulsar::Log::get_logger(), __VA_ARGS__)
#define PL_LOG_TRACE(...) SPDLOG_LOGGER_TRACE(Pulsar::Log::get_logger(), __VA_ARGS__)
