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

The sentry-native sdk is a submodule at `SentryClient/Source/ThirdParty/sentry-native`.
The binaries and header are built from this and placed in `SentryClient/Binaries/ThirdParty/sentry-native`.

- You may want to update the submodule to a new release tag.  Be sure to update the submodule
  and initialize all submodules recursively.
- You now need to rebuild the binaries and install the sdks in the proper folder.
  - Follow the instructions for each platform below
  - _install_ to the proper place, e.g. `SentryClient/Binaries/ThirdParty/sentry-native/Win64`

The sentry-native SDK uses CMake and this needs to be installed, along with any necessary platform
build tools and dependencies, mentioned below.

We build the static version of the sdk, with `none` transport, since the plugin provides
its own http transport via the unreal engine.

### Building for Windows
We must use the `RelWithDebInfo` configuration because the `Debug` configuration links with
the Debug windows CRT, which is incompatible with UE.

1. Install cmake, e.g. using `choco install cmake` (see https://chocolatey.org/ for the choco installer)
2. Open a command shell and enter the `SentryClinet/Source/ThirdParty/sentry-native` folder
3. Delete any pre-existing `build` folder
4. Run `cmake -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_SHARED_LIBS=OFF -DSENTRY_TRANSPORT=none` to configure cmake
6. Run `cmake --build build --config RelWithDebInfo` to build the binaries (do not specify --parallel, it will only build the `Debug` config)
7. Run `cmake --install build --prefix ../../../Binaries/ThirdParty/sentry-native/Win64 --config RelWithDebInfo`

### Building for Linux
This follows much the same steps as above, except that the `install` folder should be `Linux` instead of `Win64`
You need a minimum version of `CMake 3.12` for this to work.  In case of problems running the first
step, try upgrading cmake.
1. Install compilation prerequisites, such as `build-essential` along with other libs: `zlib-dev`,`libssl-dev`, `libc++-dev`,
   `libc++abi-dev`, `clang`.  This varies according to your distro.
2. `cd` to `SentryClinet/Source/ThirdParty/sentry-native`
3. Delete any pre-existing `build-linux` folder
2. Run `cmake -B build-linux -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_SHARED_LIBS=OFF -DSENTRY_TRANSPORT=none  
       -DCMAKE__COMPILER=clang -DCMAKE_CXX_COMPILER="clang++"
       -DCMAKE_CXX_FLAGS="-stdlib=libc++" -DCMAKE_EXE_LINKER_FLAGS="-stdlib=libc++"`
3. Run `cmake --build build-linux --config RelWithDebInfo --parallel`
4. Run `cmake --install build-linux --prefix ../../../Binaries/ThirdParty/sentry-native/Linux --config RelWithDebInfo`
