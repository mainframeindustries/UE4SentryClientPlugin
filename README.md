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
1. Install cmake, e.g. using `choco install cmake` (see https://chocolatey.org/ for the choco installer)
2. Open a command shell and enter the `sentry-native-release` folder, or wherever you placed your library.
3. Delete any pre-existing `build` folder
4. run `cmake -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_SHARED_LIBS=OFF` to configure cmake
6. run `cmake --build build --config RelWithDebInfo` to build the binaries (do not specify --parallel, it will only build the `Debug` config)
7. run `cmake --install build --prefix Win64 --config RelWithDebInfo` to create an install folder
8. Copy the `Win64` folder into the `SentryClient/Source/ThirdParty/sentry-native` folder

### Building for Linux
This follows the same steps as above, excep the `install` folder should be `Linux` instead of `Win64`

