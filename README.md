# UE4SentryClientPlugin
An UnrealEngine plugin for the Sentry crash reporting service


# Sentry plugin for the sentry-native sdk
This plugin implements crash handling using sentry.  It also exposes blueprint functions to communicate
with the nattive sdk

## Configuration
the usual environment varialbes such as `SENTRY_DSN` can be used.  In addition
command line flags can be used, such as -SENTRY_DSN=https://foo
An .ini file can be used as well.  The priority is (lowest to highest):
- Builtin values
- .ini file
- Env var
- command line flag.

The .ini file is  DefaultSentry.ini (or other Sentry.ini files according to the usual platform/role precedence
of UE4 ini files) under the key following key:
```
[/Script/SentryClient.SentryClientConfig]
DSN=https://YOUR_KEY@oORG_ID.ingest.sentry.io/PROJECT_ID
Release=v.0.1.9
```

##  Note:
For crash handling on windows, the Unreal Engine mush have a certain patch to disable the
Windows native Structured Execption Handling hooks that UE uses.  There is a Pull Request
active with Epic games for this (https://github.com/EpicGames/UnrealEngine/pull/7976)



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
1. Install compilation prerequisites, such as `build-essential` along with other libs: `zlib-dev`,`libssl-dev`, `libc++-dev`,
   `libc++abi-dev`, `clang`
2. Run `cmake -B build-linux -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_SHARED_LIBS=OFF -DSENTRY_TRANSPORT=none  
       -DCMAKE__COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ 
       -DCMAKE_CXX_FLAGS="-stdlib=libc++" -DCMAKE_EXE_LINKER_FLAGS="-stdlib=libc++"`
3. Run `cmake --build build-linux --config RelWithDebInfo --parallel`
4. Run `cmake --install build-linux --prefix Linux --config RelWithDebInfo`
5. Copy the `Linux` folder into the `SentryClient/Source/ThirdParty/sentry-native` folder
