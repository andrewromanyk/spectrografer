#!/bin/sh

cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE="build/generators/conan_toolchain.cmake" -DCMAKE_CONFIGURATION_TYPES=Release
cmake --build build --config Release