#!/bin/bash

#rewuires the "gcc-aarch-linux-gnu" and "g++-aarch-linux-gnu" packages
set -e
cd "$(dirname "$0")/sentry-native"
rm -rf build-linux-arm64
cmake -B build-linux-arm64 -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_SHARED_LIBS=OFF -DSENTRY_TRANSPORT=none  \
       -DSENTRY_BACKEND=crashpad \
       -DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc -DCMAKE_CXX_COMPILER="aarch64-linux-gnu-g++" \
       #-DCMAKE_CXX_FLAGS="-stdlib=libc++" -DCMAKE_EXE_LINKER_FLAGS="-stdlib=libc++"

cmake --build build-linux-arm64 --config RelWithDebInfo --parallel
cmake --install build-linux-arm64 --prefix ../../../Binaries/ThirdParty/sentry-native/LinuxArm64 --config RelWithDebInfo
