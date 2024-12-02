add_library(PulsarLibCore OBJECT
    src/PulsarCore/Log.cpp
)
FILE(GLOB_RECURSE PULSAR_LIB_CORE_FILES src/PulsarCore/*.hpp)
target_include_directories(PulsarLibCore PUBLIC src)
target_sources(PulsarLibCore INTERFACE ${PULSAR_LIB_CORE_FILES})

find_package(spdlog CONFIG REQUIRED)

target_link_libraries(PulsarLibCore PUBLIC
    spdlog::spdlog
)

add_clang_tidy(PulsarLibCore)
add_clang_format(PulsarLibCore ${PULSAR_LIB_CORE_FILES})

if (PULSAR_BUILD_TESTS)
    add_executable(PulsarLibCore_Tests
        tests/PulsarCore/GC/Pointer.cpp
        tests/PulsarCore/Result.cpp
    )
    file(GLOB_RECURSE PULSAR_LIB_CORE_TEST_FILES tests/PulsarCore/*.hpp tests/PulsarCore/*.cpp)

    add_clang_tidy(PulsarLibCore_Tests)
    add_clang_format(PulsarLibCore_Tests ${PULSAR_LIB_CORE_TEST_FILES})

    target_link_libraries(PulsarLibCore_Tests PRIVATE
        PulsarLibCore
        GTest::gtest_main
    )

    gtest_discover_tests(PulsarLibCore_Tests)
endif()
