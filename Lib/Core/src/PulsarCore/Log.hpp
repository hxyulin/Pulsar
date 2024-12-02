#pragma once

#define SPDLOG_LOG_LEVEL 0
#include <spdlog/spdlog.h>

#include "PulsarCore/Result.hpp"

namespace Pulsar {
    class Log {
    public:
        [[nodiscard]] static Result<bool, std::string> Init();
        static void                                    Shutdown();

        [[nodiscard]] inline static std::shared_ptr<spdlog::logger> GetLogger() {
            return s_Logger;
        }

    private:
        inline static std::shared_ptr<spdlog::logger> s_Logger = nullptr;
    };
} // namespace Pulsar

#define PL_LOG_FATAL(...) SPDLOG_LOGGER_CRITICAL(Pulsar::Log::GetLogger(), __VA_ARGS__)
#define PL_LOG_ERROR(...) SPDLOG_LOGGER_ERROR(Pulsar::Log::GetLogger(), __VA_ARGS__)
#define PL_LOG_WARN(...) SPDLOG_LOGGER_WARN(Pulsar::Log::GetLogger(), __VA_ARGS__)
#define PL_LOG_INFO(...) SPDLOG_LOGGER_INFO(Pulsar::Log::GetLogger(), __VA_ARGS__)
#define PL_LOG_DEBUG(...) SPDLOG_LOGGER_DEBUG(Pulsar::Log::GetLogger(), __VA_ARGS__)
#define PL_LOG_TRACE(...) SPDLOG_LOGGER_TRACE(Pulsar::Log::GetLogger(), __VA_ARGS__)
