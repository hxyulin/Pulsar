#!/usr/bin/env bash

# This is a simple script to invoke the cmake build system

# Exit on error. Append "|| true" if you expect an error.
set -e
set -u

SUBCOMMAND=${1:-build}
BUILD_DIR="build"

# We ensure that the user has cmake installed, and that we have a C++ compiler
command -v cmake >/dev/null 2>&1 || { echo "No cmake installed. Please install cmake." >&2; exit 1; }
command -v clang >/dev/null 2>&1 || { echo "No C++ compiler installed. Please install clang." >&2; exit 1; }

# Change to the directory of this script
cd "$(dirname "${BASH_SOURCE[0]}")"
# Go up one level
cd ..

if [[ "${SUBCOMMAND}" == "configure" ]]; then
    # If dependencies/vcpkg doesn't exist, clone and bootstrap vcpkg
    if [[ ! -d "dependencies/vcpkg" ]]; then
        git clone https://github.com/microsoft/vcpkg.git dependencies/vcpkg --depth 1
        cd dependencies/vcpkg
        ./bootstrap-vcpkg.sh
        ./vcpkg install
        cd ../../
    fi

    cmake -B "${BUILD_DIR}" ${@:2}
elif [[ "${SUBCOMMAND}" == "build" ]]; then
    cmake --build  "${BUILD_DIR}" ${@:2}
elif [[ "${SUBCOMMAND}" == "clean" ]]; then
    rm -rf  "${BUILD_DIR}"
    rm -rf dependencies/vcpkg dependencies/vcpkg_installed
elif [[ "${SUBCOMMAND}" == "test" ]]; then
    cd build
    ctest ${@:2} && true
    cd ..
else
    # Print help message
    echo "Usage: $0 [subcommand] [options]"
    echo "Subcommands:"
    echo "  configure - configure the project"
    echo "  build     - build the project"
    echo "  clean     - clean the project"
    echo "  test      - run the tests"

    exit 1
fi
