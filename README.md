# Reflex

C++20 cross-platform development framework for UI applications on macOS, iOS,
Windows, Android, Linux, and the web, including support for console applications
and pro-audio plugins (VST3, CLAP, VST2, AU, and AUv3).

Reflex is available in two distributions:

- **`reflex` (with source):** the framework source, headers, tools, templates,
  and build projects for building the framework and your applications.
- **`reflex_public` (prebuilt SDK):** public headers and the `reflex_ext` source,
  paired with prebuilt framework libraries.

We recommend the native ReflexCLI workflow for most projects. The CLI covers
installation, project creation, and build management, including Android
packaging and web builds that CMake alone does not fully cover.

After [installing the CLI and SDK](#installation-via-reflexcli), create a project
outside the SDK folder:

```text
mkdir MyApp
cd MyApp
reflex create
```

Follow the prompts to choose a template, enter the vendor and product names,
and select the current folder as the output location. `reflex create` creates
`project.cfg` and the template's source files and assets, then generates the
platform projects automatically. Run `reflex build` to build for the host's
native platform, or `reflex edit` to open a generated project in its IDE.

To add source files to a project, update the `@Files` list in `project.cfg`, then
run `reflex generate` to regenerate the platform projects.

For a graphical project-creation interface, optionally build **Reflex Project
Creator**. With the CLI and SDK installed, run these commands from the SDK root:

```text
cd build/tools/ReflexProjectCreator
reflex generate
reflex build release
```

Launch the resulting application from the platform's Release subfolder under
`build/tools/ReflexProjectCreator/output/`.

CMake is also available for projects with established CMake workflows or
CMake-based dependencies.

**AI agents:** Read `AGENTS.md` before working with the SDK.

## Installation via ReflexCLI

### Prebuilt SDK (`reflex_public`)

Bootstrap the CLI using the download script for your platform.
Run the script below with your preferred install location as the final argument.
The script creates a `reflex_public/` folder in that directory and adds the CLI
to your `PATH`.

macOS:

```sh
curl -fsSL https://reflexplusplus.dev/install.sh | sh -s -- ~/dev
```

Windows (PowerShell):

```powershell
& ([scriptblock]::Create((irm https://reflexplusplus.dev/install.ps1))) C:\dev
```

Restart Terminal or PowerShell, then install the SDK and prebuilt binaries for
the host platform:

```sh
reflex install
```

Use `reflex install --platforms <platform[,..]>` to request binaries for other
platforms, for example `reflex install --platforms macos,ios`.

For more information, see
[Download Reflex++](https://reflexplusplus.dev/download).

### Source checkout (`reflex`)

With CMake 3.22+ and the platform's compiler toolchain installed, build the CLI
from the repository root using the provided script:

macOS:

```sh
./build/tools/ReflexCLI/build_release.command
```

Windows (PowerShell):

```powershell
.\build\tools\ReflexCLI\build_release.bat
```

The scripts install the executable into `bin/tools/macos/` or `bin/tools/win/`.
Add that folder to your `PATH`.

To pre-build the framework libraries:

```text
cd path/to/reflex/build/lib
reflex generate
reflex build
```

This builds the host's native platform. Use `reflex build all` to build all
enabled platforms in the host's default selection; each requires its own
toolchain. To select a platform and configuration, use a command such as
`reflex build windows debug`.

Pre-building the libraries is optional. With the stock configs in a source
checkout, running `reflex create` followed by `reflex build` builds your project
and its required framework dependencies automatically.

The optional setting `reflex set reflex_source true` includes the framework
libraries as source targets in the application's generated project, making
their source visible in the IDE and building them alongside the application.
Without this setting, building directly from the application's IDE project
expects the libraries to have been built already. Regenerate after changing
the setting.

For more details, see
[Reflex Generate and Build System](documentation/reflex-build.md).

## Installation via CMake

### Requirements

- CMake **3.22+**
- A C++20 compiler and platform SDK from the
  [platform toolchain requirements](documentation/toolchain.md)

### FetchContent

The CMake examples below assume you already have application sources and a
`resources.xml` file, such as those provided by the project templates. Adjust
the source lists to match your project.

```cmake
cmake_minimum_required(VERSION 3.22)
project(MyPlugin LANGUAGES C CXX)

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
    SOURCES code/entry.cpp code/instance.cpp code/view.cpp code/resources.cpp
            ${REFLEX_ROOT}/src/reflex_ext.cpp
    NAME    "My Plugin"
    VENDOR  "My Company"
    VERSION "1.0.0"
    PACKAGE_ID_VENDOR  "mycompany"
    PACKAGE_ID_PRODUCT "myplugin"
    AU_COMPONENTS "MyPl:aufx:My Plugin" # subtype:type[:name]; separate components with commas
    AU_VENDOR_4CC "MyCo")
```

With `reflex_public`, this downloads the matching prebuilt libraries for your
platform and exposes the `Reflex::*` targets and `reflex_add_*` helpers. Pin
`REFLEX_VERSION` to a release tag so the headers and downloaded libraries
always match.

### CPM

This example assumes CPM.cmake is available at `cmake/CPM.cmake` in your project.

```cmake
cmake_minimum_required(VERSION 3.22)
project(MyApp LANGUAGES C CXX)

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
    SOURCES code/app.cpp code/entry.cpp code/view.cpp code/resources.cpp
            ${REFLEX_ROOT}/src/reflex_ext.cpp
    NAME "My App" VENDOR "My Company")
```

### Local SDK checkout

If you keep a checkout of the SDK on disk, skip FetchContent and point
`find_package` at it. This works with either distribution:

```cmake
find_package(Reflex REQUIRED CONFIG
    PATHS "/path/to/reflex/cmake"
    NO_DEFAULT_PATH)
```

A full source checkout also provides framework sources. CMake's
`REFLEX_BUILD_TARGETS_FROM_SOURCE` option defaults to `ON` when those sources
are present. The audio-plugin helper uses it to build available platform target
libraries from source, falling back to prebuilt libraries when a source is
absent. Common libraries still use prebuilt binaries. The CLI's `reflex_source`
setting is separate from this CMake option.

## Helper functions

All accept `SOURCES`, `NAME`, and `VENDOR`, plus format-specific options where
applicable. They configure platform libraries, compiler flags, and bundle/plist
generation where needed.
Add `${REFLEX_ROOT}/src/reflex_ext.cpp` to `SOURCES` for every target.

| Helper | Builds |
|---|---|
| `reflex_add_app` | GUI application |
| `reflex_add_vm_app` | VM-driven GUI application (requires an SDK with VM support) |
| `reflex_add_audio_plugin` | One target per `FORMATS` entry (Standalone, VST3, CLAP, VST2, AU, AUv3, AAX) |
| `reflex_add_console_app` | command-line application |

Formats unavailable on the current platform are skipped automatically.
AUv3 and AAX builds require their framework sources; AAX also requires the AAX
SDK. The public SDK does not include VM support.

## Imported targets

`Reflex::Common`, `Reflex::CommonUi`, `Reflex::Vm`,
`Reflex::TargetApp`, `Reflex::TargetAudioApp`, `Reflex::TargetConsole`,
`Reflex::TargetLibrary`, `Reflex::TargetDynamicLibrary`, and the format targets
`Reflex::TargetVST3`, `Reflex::TargetCLAP`, `Reflex::TargetVST2`, and
`Reflex::TargetAU`.

Targets are defined only when their libraries are present in the SDK and
supported on the selected platform. The helpers link them for you; you rarely
need to reference them directly.

## Platform toolchains

See [Platform Toolchains](documentation/toolchain.md) for the current
platform-specific requirements and setup notes.

## License

See [LICENSE.txt](LICENSE.txt).
