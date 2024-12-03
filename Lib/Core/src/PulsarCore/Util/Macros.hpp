#pragma once

// Helper struct for RUN_ONCE
namespace Pulsar::internal {
    // NOLINTNEXTLINE(readability-identifier-naming)
    struct _once_flag_t {
        // TODO: Use std::atomic<bool> ior std::once_flag
        bool m_Once = false;

        bool test_and_set() {
            if (m_Once) {
                return true;
            }
            m_Once = true;
            return false;
        }
    };
}  // namespace Pulsar::internal

/// Macro to run a block of code only once, usage:
/// PULSAR_RUN_ONCE {
///     // code
/// }
#define PULSAR_RUN_ONCE \
    static ::Pulsar::internal::_once_flag_t __once_flag##__LINE__; \
    if (!__once_flag##__LINE__.test_and_set())

#ifdef NDEBUG
    #define PULSAR_DEBUG_RUN_ONCE if (false)
#else
    #define PULSAR_DEBUG_RUN_ONCE PULSAR_RUN_ONCE
#endif

template<typename... Args>
// NOLINTNEXTLINE(readability-identifier-naming, readability-named-parameter, hicpp-named-parameter)
constexpr void PULSAR_UNUSED([[maybe_unused]] Args&&...) {
}

template<typename T>
// NOLINTNEXTLINE(readability-identifier-naming, readability-named-parameter, hicpp-named-parameter)
constexpr void PULSAR_IGNORE_RESULT([[maybe_unused]] T&&) {
}
