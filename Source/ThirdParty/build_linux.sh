#!/bin/bash

set -e
pushd "$(dirname "$0")/sentry-native"
rm -rf build-linux
cmake -B build-linux -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_SHARED_LIBS=OFF -DSENTRY_TRANSPORT=none  \
       -DSENTRY_BACKEND=crashpad \
       -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER="clang++" \
       -DCMAKE_CXX_FLAGS="-stdlib=libc++" -DCMAKE_EXE_LINKER_FLAGS="-stdlib=libc++"

cmake --build build-linux --config RelWithDebInfo --parallel
cmake --install build-linux --prefix ../../../Binaries/ThirdParty/sentry-native/Linux --config RelWithDebInfo
popd
