#include "PulsarCore/Types.hpp"

using Pulsar::i8, Pulsar::u8, Pulsar::i16, Pulsar::u16, Pulsar::i32, Pulsar::u32, Pulsar::i64,
    Pulsar::u64, Pulsar::f32, Pulsar::f64;

static_assert(sizeof(i8) == sizeof(u8));
static_assert(sizeof(i16) == sizeof(u16));
static_assert(sizeof(i32) == sizeof(u32));
static_assert(sizeof(i64) == sizeof(u64));

static_assert(sizeof(i8) == 1);
static_assert(sizeof(u8) == 1);
static_assert(sizeof(i16) == 2);
static_assert(sizeof(u16) == 2);
static_assert(sizeof(i32) == 4);
static_assert(sizeof(u32) == 4);
static_assert(sizeof(i64) == 8);
static_assert(sizeof(u64) == 8);

static_assert(sizeof(f32) == 4);
static_assert(sizeof(f64) == 8);
