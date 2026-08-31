# Reflex

C++20 cross-platform framework for building audio plugins (VST3, CLAP, VST2, AU,
AUv3) and GUI applications. Targets macOS, iOS, Windows, Linux, and WebAssembly.

This repository provides the public headers, CMake integration, and the
`reflex_ext` source. Reflex++ also requires prebuilt static libraries, which can
be installed via the ReflexCLI or fetched by CMake.

Unless you already have a CMake-based project and its dependencies in place, we
recommend the native ReflexCLI workflow: use `reflex install` to install the SDK
and prebuilt dependencies, then use `reflex create` and `reflex generate` to
create and generate projects for all supported platforms.

## Installation via ReflexCLI

### Bootstrap the CLI

Open Terminal (macOS) or PowerShell (Windows) and cd to your preferred install location. 
The bootstrap script creates a reflex_public/ folder in the current working directory and 
adds it to your PATH.

macOS:

```sh
cd ~/dev
curl -fsSL https://reflexplusplus.dev/install/macos/bootstrap.sh | sh
```

Windows (PowerShell):

```powershell
cd C:\dev
irm https://reflexplusplus.dev/install/win/bootstrap.ps1 | iex
```

Restart Terminal or PowerShell, then install the SDK and prebuilt binaries:

```sh
reflex install
```

**AI agents:** After installation, be sure to read AGENTS.md before working with the SDK.

For more information, see
[Download Reflex++](https://reflexplusplus.dev/download).

## Installation via CMake

### Requirements

- CMake **3.22+**
- A C++20 compiler (Apple Clang, MSVC 2022, or Clang/GCC on Linux)

### FetchContent

```cmake
include(FetchContent)
set(REFLEX_VERSION "v0.3.19")

FetchContent_Declare(reflex
    GIT_REPOSITORY https://github.com/reflexplusplus/reflex_public.git
    GIT_TAG        ${REFLEX_VERSION}
    SOURCE_SUBDIR  nonexistent) # find_package it, don't add as a subproject
FetchContent_MakeAvailable(reflex)

find_package(Reflex REQUIRED CONFIG
    PATHS "${reflex_SOURCE_DIR}/cmake"
    NO_DEFAULT_PATH)

reflex_add_audio_plugin(MyPlugin
    FORMATS Standalone VST3 CLAP AU
    SOURCES code/entry.cpp code/instance.cpp code/view.cpp
            ${REFLEX_ROOT}/src/reflex_ext.cpp
    NAME    "My Plugin"
    VENDOR  "My Company"
    VERSION "1.0.0"
    PACKAGE_ID_VENDOR  "mycompany"
    PACKAGE_ID_PRODUCT "myplugin"
    AU_COMPONENTS "MyPl:aufx:My Plugin" # subtype:type[:name]; separate components with commas
    AU_VENDOR_4CC "MyCo")
```

This downloads the matching prebuilt libraries for your platform and exposes
the `Reflex::*` targets and `reflex_add_*` helpers. Pin `REFLEX_VERSION` to a
release tag so the source and downloaded libraries always match.

### CPM

```cmake
include(cmake/CPM.cmake)
set(REFLEX_VERSION "v0.3.19")

CPMAddPackage(
    NAME           reflex
    GIT_REPOSITORY https://github.com/reflexplusplus/reflex_public.git
    GIT_TAG        ${REFLEX_VERSION}
    DOWNLOAD_ONLY  YES)

find_package(Reflex REQUIRED CONFIG
    PATHS "${reflex_SOURCE_DIR}/cmake"
    NO_DEFAULT_PATH)

reflex_add_app(MyApp
    SOURCES code/app.cpp code/entry.cpp ${REFLEX_ROOT}/src/reflex_ext.cpp
    NAME "My App" VENDOR "My Company")
```

### Local SDK checkout

If you keep a checkout of the SDK on disk, skip FetchContent and point
`find_package` at it:

```cmake
find_package(Reflex REQUIRED CONFIG
    PATHS "/path/to/reflex/cmake"
    NO_DEFAULT_PATH)
```

## Helper functions

All take `SOURCES`, `NAME`, `VENDOR` (+ format/AU options where relevant) and set
up the platform frameworks, bundle/plist generation, and compiler flags for you.
Add `${REFLEX_ROOT}/src/reflex_ext.cpp` to `SOURCES` for every target.

| Helper | Builds |
|---|---|
| `reflex_add_app` | GUI application |
| `reflex_add_vm_app` | VM-driven GUI application |
| `reflex_add_audio_plugin` | one target per `FORMATS` entry (Standalone VST3 CLAP VST2 AU AUv3) |
| `reflex_add_console_app` | command-line application |

Formats unavailable on the current platform are skipped automatically.

## Imported targets

`Reflex::Common`, `Reflex::CommonUi`, `Reflex::Vm`, `Reflex::VmUi`,
`Reflex::TargetApp`, `Reflex::TargetAudioApp`, `Reflex::TargetConsole`,
`Reflex::TargetLibrary`, `Reflex::TargetDynamicLibrary`, and the per-format
`Reflex::TargetVST3 / TargetCLAP / TargetVST2 / TargetAU` (platform dependent).
The helpers link these for you; you rarely reference them directly.

## License

See [LICENSE.txt](LICENSE.txt).
