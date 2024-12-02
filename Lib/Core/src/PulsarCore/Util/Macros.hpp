#pragma once

#define PULSAR_RUN_ONCE(...) \
    static bool __once##__LINE__ = false; \
    if (!__once##__LINE__) { \
        __once##__LINE__ = true; \
        __VA_ARGS__; \
    }
