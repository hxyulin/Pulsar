#pragma once


// If clang and C++23, use std::expected
#if __has_include(<expected>)
    #include <expected>
namespace Pulsar {
    template<typename T, typename Err> using Result = std::expected<T, Err>;
    template<typename Err> using Err                = std::unexpected<Err>;
} // namespace Pulsar
#else
    #error "std::expected not found"
#endif
