#pragma once

#include <cstddef>
#include <cstdint>

namespace Pulsar {
    using i8    = std::int8_t;
    using u8    = std::uint8_t;
    using i16   = std::int16_t;
    using u16   = std::uint16_t;
    using i32   = std::int32_t;
    using u32   = std::uint32_t;
    using i64   = std::int64_t;
    using u64   = std::uint64_t;
    using f32   = float;
    using f64   = double;
    using usize = size_t;

    // NOLINTBEGIN(readability-identifier-naming)
    template<typename T> struct Vector2_t {
        T x;
        T y;
    };

    template<typename T> struct Vector3_t {
        T x;
        T y;
        T z;
    };

    template<typename T> struct Vector4_t {
        T x;
        T y;
        T z;
        T w;
    };

    using IVec2 = Vector2_t<i32>;
    using IVec3 = Vector3_t<i32>;
    using IVec4 = Vector4_t<i32>;

    using UVec2 = Vector2_t<u32>;
    using UVec3 = Vector3_t<u32>;
    using UVec4 = Vector4_t<u32>;

    using Vec2 = Vector2_t<f32>;
    using Vec3 = Vector3_t<f32>;
    using Vec4 = Vector4_t<f32>;

    // NOLINTEND(readability-identifier-naming)
} // namespace Pulsar
