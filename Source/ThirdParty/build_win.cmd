
rem Builds the binaries for windows.  See the Readme.md in the root for details.

rem First build the crashpad version
pushd %~dp0\sentry-native
rem Remove the `build` folder first
rmdir /s /q build-win-crashpad
rem configure cmake build
cmake -G "Visual Studio 17 2022" -A x64 -B build-win-crashpad -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_SHARED_LIBS=OFF -DSENTRY_TRANSPORT=none
rem build the files (no parallel, that doesn't work)
cmake --build build-win-crashpad --config RelWithDebInfo
rem install the binaries
cmake --install build-win-crashpad --prefix ../../../Binaries/ThirdParty/sentry-native/Win64-Crashpad --config RelWithDebInfo

rem Now build the breakpad version
rem Remove the `build` folder first
rmdir /s /q build-win-breakpad
rem configure cmake build
cmake -G "Visual Studio 17 2022" -A x64 -B build-win-breakpad -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_SHARED_LIBS=OFF -DSENTRY_TRANSPORT=none -DSENTRY_BACKEND=breakpad
rem build the files (no parallel, that doesn't work)
cmake --build build-win-breakpad --config RelWithDebInfo
rem install the binaries
cmake --install build-win-breakpad --prefix ../../../Binaries/ThirdParty/sentry-native/Win64-Breakpad --config RelWithDebInfo

popd