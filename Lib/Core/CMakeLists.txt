add_library(PulsarLibCore STATIC
    src/PulsarCore/Log.cpp
)
FILE(GLOB_RECURSE PULSAR_LIB_CORE_FILES src/PulsarCore/*.hpp src/PulsarCore/*.cpp)
target_include_directories(PulsarLibCore PUBLIC src)
target_sources(PulsarLibCore PUBLIC ${PULSAR_LIB_CORE_FILES})

find_package(spdlog CONFIG REQUIRED)

target_link_libraries(PulsarLibCore PUBLIC
    spdlog::spdlog
)

add_clang_tidy(PulsarLibCore)
add_clang_format(PulsarLibCore ${PULSAR_LIB_CORE_FILES})

if (PULSAR_BUILD_TESTS)
    add_executable(PulsarLibCore_Tests
        tests/PulsarCore/GC/Pointer.cpp
        tests/PulsarCore/GC/Allocators/Arena.cpp
        tests/PulsarCore/Result.cpp
        tests/PulsarCore/Types.cpp
    )
    file(GLOB_RECURSE PULSAR_LIB_CORE_TEST_FILES tests/PulsarCore/*.hpp tests/PulsarCore/*.cpp)

    # Explicitly disable clang-tidy for tests
    # We don't want to include clang-tidy in the tests, due to many false positives and redundant checks
    add_clang_tidy(PulsarLibCore_Tests OFF)
    add_clang_format(PulsarLibCore_Tests ${PULSAR_LIB_CORE_TEST_FILES})

    target_link_libraries(PulsarLibCore_Tests PRIVATE
        PulsarLibCore
        GTest::gtest_main
    )

    gtest_discover_tests(PulsarLibCore_Tests)
endif()

if (PULSAR_BUILD_BENCHMARKS)
    add_executable(PulsarLibCore_Benchmarks
        benchmarks/PulsarCore/Allocator.cpp
    )
    file(GLOB_RECURSE PULSAR_LIB_CORE_BENCHMARK_FILES benchmarks/PulsarCore/*.hpp benchmarks/PulsarCore/*.cpp)

    target_link_libraries(PulsarLibCore_Benchmarks PRIVATE
        PulsarLibCore
        benchmark::benchmark
    )

    add_clang_tidy(PulsarLibCore_Benchmarks OFF)
    add_clang_format(PulsarLibCore_Benchmarks ${PULSAR_LIB_CORE_BENCHMARK_FILES})

    target_include_directories(PulsarLibCore_Benchmarks PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/benchmarks)
endif()
