
rem Builds the binaries for windows.  See the Readme.md in the root for details.

cd %~dp0\sentry-native
rem Remove the `build` folder first
rmdir /s build
rem configure cmake build
cmake -G "Visual Studio 16 2019" -A x64 -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_SHARED_LIBS=OFF -DSENTRY_TRANSPORT=none
rem build the files (no parallel, that doesn't work)
cmake --build build --config RelWithDebInfo
rem install the binaries
cmake --install build --prefix ../../../Binaries/ThirdParty/sentry-native/Win64 --config RelWithDebInfo