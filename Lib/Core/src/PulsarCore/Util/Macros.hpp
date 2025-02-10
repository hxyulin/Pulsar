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

#if not defined(NDEBUG) or defined(PULSAR_FORCE_DEBUG)
    #define PULSAR_DEBUG
#else
    #define PULSAR_RELEASE
#endif

/// Macro to run a block of code only once, usage:
/// PULSAR_RUN_ONCE {
///     // code
/// }
#define PULSAR_RUN_ONCE \
    static ::Pulsar::internal::_once_flag_t __once_flag##__LINE__; \
    if (!__once_flag##__LINE__.test_and_set())

#ifdef PULSAR_DEBUG
    #define PULSAR_DEBUG_RUN_ONCE PULSAR_RUN_ONCE
    #define PULSAR_DEBUG_ONLY if (true)
#else
    #define PULSAR_DEBUG_RUN_ONCE if (false)
    #define PULSAR_DEBUG_ONLY if (false)
#endif


// We are using clang, so we can just use clang's attributes
#define PULSAR_ALWAYS_INLINE [[gnu::always_inline]]
#define PULSAR_NO_INLINE [[gnu::noinline]]
#define PULSAR_NO_RETURN [[noreturn]]

template<typename... Args>
// NOLINTNEXTLINE(readability-identifier-naming, readability-named-parameter, hicpp-named-parameter)
PULSAR_ALWAYS_INLINE constexpr void PULSAR_UNUSED([[maybe_unused]] Args&&...) {
}

template<typename T>
// NOLINTNEXTLINE(readability-identifier-naming, readability-named-parameter, hicpp-named-parameter)
PULSAR_ALWAYS_INLINE constexpr void PULSAR_IGNORE_RESULT([[maybe_unused]] T&&) {
}

// User can define their own assertion macro
#ifndef PULSAR_ASSERT
    #define PULSAR_ASSERT(expr, ...)
#endif
