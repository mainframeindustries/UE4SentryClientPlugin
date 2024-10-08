# UE4SentryClientPlugin
An UnrealEngine plugin for the Sentry crash reporting service (https://github.com/getsentry/sentry-native)

# Sentry plugin for the sentry-native sdk
This plugin implements crash handling using sentry.  It also exposes blueprint functions to communicate
with the native sdk 

## Platforms
This plugin is supported on Windows and Linux.  For other platforms, the plugin provides
a dummy implementation.

## Configuration
the usual environment variables such as `SENTRY_DSN` can be used.  In addition
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
DSN="https://YOUR_KEY@oORG_ID.ingest.sentry.io/PROJECT_ID"
Release="v.0.1.9"
```

The current list of config keys:
| .ini file         | Environment variable      | Command line flag          |
|-------------------|---------------------------|----------------------------|
| `DSN`             | `SENTRY_DSN`              | `-SENTRY_DSN`              |
| `Environment`     | `SENTRY_ENVIRONMENT`      | `-SENTRY_ENVIRONMENT`      |
| `Release`         | `SENTRY_RELEASE`          | `-SENTRY_RELEASE`          |
|`ConsentRequired`  | `SENTRY_CONSENT_REQUIRED` | `-SENTRY_CONSENT_REQUIRED` |

All take a value, such as
```sh
export SENTRY_RELEASE=GoldMaster3000
```
or
```sh
SuperDuperGame.exe -SENTRY_ENVIRONMENT=TENTATIVE_DEBUG -SENTRY_CONSENT_REQUIRED=1
```

##  Note:
For crash handling on Windows, UnrealEngine 5.1 or later must be used.  This version
allows the plugin to take over crash handling from the engine's own handlers.

For an older engine, a patch needs to be applied to the engine source code.
There is a Pull Request for UnrealEngine which accomplishes this:
https://github.com/EpicGames/UnrealEngine/pull/7976)
If your engine is patched in that way, you can  `#define HAVE_CRASH_HANDLING_THING 1`
in the file `SentryClientModule.cpp`

## Run-time requirements

The `crashpad` backend used for Linux requires C++ runtime to be installed.  Use `apt install libc++`
or similar to install it.  Currently the `sentry-native` package does not support static linking of
this executable.

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

For windows, we need to build both Crashpad and Breakpad enabled versions of the library since the
plugin can select, at compilation time, which backend to use.

A batch file compiling both versions is available at `Source/ThirdParty/build_win.cmd`

#### Crashpad

The following are the steps needed to build the crashpad-backend.

We must use the `RelWithDebInfo` configuration because the `Debug` configuration links with
the Debug windows CRT, which is incompatible with UE. UnrealEngine 4.26 uses VisualStudio 2017 as the official code generator.  Newer versions of VisualStudio create code that cannot be linked with an engine built with 2017.

1. Install cmake, e.g. using `choco install cmake --installargs 'ADD_CMAKE_TO_PATH=System'` (see https://chocolatey.org/ for the choco installer)
2. Open a command shell and enter the `Source/ThirdParty/sentry-native` folder
3. Delete any pre-existing `build-win-crashpad` folder
4. Run `cmake -G "Visual Studio 17 2022" -A x64 -B build-win-crashpad -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_SHARED_LIBS=OFF -DSENTRY_TRANSPORT=none` to configure cmake
6. Run `cmake --build build-win-crashpad --config RelWithDebInfo` to build the binaries (do not specify --parallel, it will only build the `Debug` config)
7. Run `cmake --install build-win-crashpad --prefix ../../../Binaries/ThirdParty/sentry-native/Win64-Crashpad --config RelWithDebInfo`

#### Breakpad

Building Breakpad binaries is similar, but we keep a separate build foler, `build-win-breakpad` and install
the binaris in a `Win64-Breakpad` folder.  This is automatically done as part of the `build_win.cmd` script.

### Building for Linux
This follows much the same steps as above, except that the `install` folder should be `Linux` instead of `Win64`
You need a minimum version of `CMake 3.12` for this to work.  In case of problems running the first
step, try upgrading cmake.  We will use the `crashpad` backend for linux, instead of the default `breakpad` since the
crashpad handler is out of process and uploads immediately, rather than during the next run.  This is important for linux servers, particulary in containers, where the server may not be run again in the same place (and thus, the delayed uploading of the crash will not occur.)

Another thing to consider is that for Unreal, you want to be using a Ubuntu-18 / CentOS7 compatible environment
so that you have the same run-time requirements (glibc) as the unreal game you are targetting.  For windows users, using a
WSL environment with **Ubuntu-18.04** and installing the latest cmake on it will be adequate. A `Dockerfile` is supplied which assembles the correct build environment.

1. Install compilation prerequisites, such as `build-essential` along with other libs: `libz-dev`, `libc++-dev`,
   `libc++abi-dev`, `clang`, `libcurl4-openssl-dev`.  This varies according to your distro.  See `CONTRIBUTING.md` in `sentry-native` for info.
2. `cd` to `Source/ThirdParty/sentry-native`
3. Delete any pre-existing `build-linux` folder
2. Run `cmake -B build-linux -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_SHARED_LIBS=OFF -DSENTRY_TRANSPORT=none  \
       -DSENTRY_BACKEND=crashpad \
       -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER="clang++" \
       -DCMAKE_CXX_FLAGS="-stdlib=libc++" -DCMAKE_EXE_LINKER_FLAGS="-stdlib=libc++"`
3. Run `cmake --build build-linux --config RelWithDebInfo --parallel`
4. Run `cmake --install build-linux --prefix ../../../Binaries/ThirdParty/sentry-native/Linux --config RelWithDebInfo`

A shell script which performs the above is available in `Source/ThirdParty/build_linux.sh`.  To automatically use a custom *Docker* image for the build tools, use `Source/ThirdParty/build_linux_docker.sh`.

**Note:**  the `crashpad_handler` requires the `libunwind` shared library to be installed on the machine at runtime.  You can
install it with something akin to `apt install libunwind8`.
