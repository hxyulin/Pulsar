# The directory structure is as follows:
#   Pulsar
#   ├── CMakeLists.txt
#   ├── LICENSE
#   ├── Lib
#   │   ├── Core - Core library
#   │   ├── Window - Window library
#   │   └── Renderer - Renderer library
#   ├── Editor
#   └── Engine

cmake_minimum_required(VERSION 3.25)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Configure VCPKG
set(VCPKG_MANIFEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/dependencies")
set(CMAKE_TOOLCHAIN_FILE "${VCPKG_MANIFEST_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake")
set(VCPKG_INSTALL_OPTIONS "--clean-after-build")
set(VCPKG_BINARY_SOURCES "clear;default,readwrite")
set(VCPKG_MANIFEST_MODE ON)
set(VCPKG_MANIFEST_QUIET ON)

project(
    Pulsar
    LANGUAGES CXX
)

# We only support clang
if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND NOT CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    message(FATAL_ERROR "Only clang is supported")
endif()

# Although not fully supported, we can use C++23 features, like std::expected
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -Werror")

set(PULSAR_BUILD_TESTS ON CACHE BOOL "Build tests")
set(PULSAR_CLANG_TIDY ON CACHE BOOL "Run clang-tidy")
set(PULSAR_CLANG_FORMAT ON CACHE BOOL "Run clang-format")

if (PULSAR_BUILD_TESTS)
    enable_testing()
    find_package(GTest REQUIRED)
    include(GoogleTest)
endif()

if (PULSAR_CLANG_TIDY)
    find_program(CLANG_TIDY clang-tidy)
    if (CLANG_TIDY)
        set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY})
    else()
        message(WARNING "clang-tidy not found")
        set(PULSAR_CLANG_TIDY OFF CACHE BOOL "Run clang-tidy" FORCE)
    endif()
endif()

if (PULSAR_CLANG_FORMAT)
    find_program(CLANG_FORMAT clang-format)
    if (CLANG_FORMAT)
        set(CMAKE_CXX_CLANG_FORMAT ${CLANG_FORMAT})
    else()
        message(WARNING "clang-format not found")
        set(PULSAR_CLANG_FORMAT OFF CACHE BOOL "Run clang-format" FORCE)
    endif()

    # The targets will be added as dependencies to the format target
    add_custom_target(format ALL)
endif()

function(add_clang_tidy target)
    if (PULSAR_CLANG_TIDY)
        set_target_properties(${target} PROPERTIES CXX_CLANG_TIDY "${CMAKE_CXX_CLANG_TIDY}")
    endif()
endfunction()

function(add_clang_format target)
    if (PULSAR_CLANG_FORMAT)
        add_custom_target(format-${target})
        add_custom_command(
            TARGET format-${target}
            PRE_BUILD
            COMMAND ${CMAKE_CXX_CLANG_FORMAT} "-i" ${ARGN}
        )
        add_dependencies(format format-${target})
    endif()
endfunction()

# Set output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(Lib)
add_subdirectory(Editor)
