# UE4SentryClientPlugin
An UnrealEngine plugin for the Sentry crash reporting service



## Updating the sentry-native binaries
1. Get the latest release from https://github.com/getsentry/sentry-native
2. Unzip the contents into `SentryClient/Source/ThirdParty/sentry-native-release`
3. Build a release in there, for each platform (see below)
4. Copy the platform specific folder in to `SentryClient/Source/ThirdParty/sentry-native/`, e.g. `Win64` or `Linux`


### Building for Windows
We must use the `RelWithDebInfo` configuration because the `Debug` configuration links with
the Debug windows CRT, which is incompatible with UE.
We also provide a transport, using http, so we use the `none` transport.

1. Install cmake, e.g. using `choco install cmake` (see https://chocolatey.org/ for the choco installer)
2. Open a command shell and enter the `sentry-native-release` folder, or wherever you placed your library.
3. Delete any pre-existing `build` folder
4. Run `cmake -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_SHARED_LIBS=OFF -DSENTRY_TRANSPORT=none` to configure cmake
6. Run `cmake --build build --config RelWithDebInfo` to build the binaries (do not specify --parallel, it will only build the `Debug` config)
7. Run `cmake --install build --prefix Win64 --config RelWithDebInfo` to create an install folder
8. Copy the `Win64` folder into the `SentryClient/Source/ThirdParty/sentry-native` folder

### Building for Linux
This follows much the same steps as above, except that the `install` folder should be `Linux` instead of `Win64`
You need a minimum version of `CMake 3.12` for this to work.  In case of problems running the first
step, try upgrading cmake.
1. Install prerequisites, zlib-dev, and libssl-dev
2. Run `cmake -B build-linux -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_SHARED_LIBS=OFF -DSENTRY_TRANSPORT=none`
3. Run `cmake --build build-linux --config RelWithDebInfo --parallel`
4. Run `cmake --install build-linux --prefix Linux --config RelWithDebInfo`
5. Copy the `Linux` folder into the `SentryClient/Source/ThirdParty/sentry-native` folder
