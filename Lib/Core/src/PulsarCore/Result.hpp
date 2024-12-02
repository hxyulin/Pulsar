#pragma once

namespace Pulsar {
// If clang and C++23, use std::expected
#if __has_include(<expected>)
    #include <expected>
    template<typename T> using Result = std::expected<T>;
#else
    #error "std::expected not found"
#endif
} // namespace Pulsar
