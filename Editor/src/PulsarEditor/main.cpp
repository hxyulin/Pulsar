#include "PulsarCore/Log.hpp"

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
    auto res = Pulsar::Log::init();
    if (!res.has_value()) {
        PL_LOG_ERROR("Failed to initialize logging: {}", res.error());
        return 1;
    }
    PL_LOG_INFO("Starting Pulsar Editor...");
    return 0;
}
