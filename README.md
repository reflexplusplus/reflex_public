# Reflex

C++20 cross-platform framework for building audio plugins (VST3, CLAP, VST2, AU,
AUv3) and GUI applications. Targets macOS, iOS, Windows, Linux, and WebAssembly.

This repository ships the public headers, CMake integration, and a small amount
of source. Prebuilt static libraries for each platform are attached to the
[GitHub Releases](https://github.com/reflexplusplus/reflex_public/releases) and
are **downloaded automatically** the first time you configure — there is no
manual install step.

## Requirements

- CMake **3.22+**
- A C++20 compiler (Apple Clang, MSVC 2022, or Clang/GCC on Linux)

## Add Reflex to your project

### FetchContent (recommended)

```cmake
include(FetchContent)
FetchContent_Declare(reflex
    GIT_REPOSITORY https://github.com/reflexplusplus/reflex_public.git
    GIT_TAG        v0.3.1        # pin a release tag
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(reflex)

reflex_add_audio_plugin(MyPlugin
    FORMATS Standalone VST3 CLAP AU
    SOURCES code/entry.cpp code/instance.cpp code/view.cpp
            ${REFLEX_ROOT}/src/reflex_ext.cpp
    NAME    "My Plugin"
    VENDOR  "My Company"
    VERSION "1.0.0"
    PACKAGE_ID_VENDOR  "mycompany"
    PACKAGE_ID_PRODUCT "myplugin"
    AU_TYPE_4CC   "aufx"     # aumu=instrument, aumi=MIDI fx, aufx=effect
    AU_UID_4CC    "MyPl"
    AU_VENDOR_4CC "MyCo")
```

`FetchContent_MakeAvailable(reflex)` downloads the matching prebuilt libraries
for your platform from the `v0.3.1` release, then exposes the `Reflex::*` targets
and the `reflex_add_*` helpers. Pin `GIT_TAG` to a release tag so the source and
the downloaded libraries always match.

### CPM

```cmake
CPMAddPackage("gh:reflexplusplus/reflex_public@0.3.1")

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
