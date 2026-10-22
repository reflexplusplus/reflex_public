# Platform Toolchains

Reflex++ uses C++20. Install toolchains separately from the SDK. Generate the
selected output before building; see [the build workflow](reflex-build.md).

## Reflex Build

`reflex build <platform>` uses the platform's native generated build system:
Visual Studio/MSBuild on Windows, Xcode on Apple platforms, Gradle on Android,
Make on Linux, and the direct Clang/WASI workflow on Web.

### macOS

Use **Xcode 16+** (Apple Clang 16+) with the macOS SDK. Native projects use
Xcode.

[Xcode system requirements](https://developer.apple.com/xcode/system-requirements/)

### iOS

Use **Xcode 16+** with the iOS SDK. Device builds also require signing
configuration.

[Xcode system requirements](https://developer.apple.com/xcode/system-requirements/)

### Windows

Use **Visual Studio 2022 or 2026** with the **Desktop development with C++**
workload, MSVC, and a Windows SDK. Use an up-to-date installation.

[Visual Studio downloads](https://visualstudio.microsoft.com/downloads/)

### Android

Use **Android Studio compatible with AGP 9.2**. Through SDK Manager, install
**Android SDK Platform 37.0**, **SDK Build Tools 36.0.0**,
**NDK (Side by side) 28.2.13676358**, and **CMake 3.22.1**. The command-line
SDK package identifier for the platform is `platforms;android-37.0`; generated
projects use API level 37.

Use **JDK 17** or a compatible Android Studio bundled JDK, together with the
generated Gradle wrapper (currently Gradle 9.4.1 / AGP 9.2.0).
Command-line builds need the JDK available through `JAVA_HOME` and
the SDK through `ANDROID_HOME` or the generated project's `local.properties`.

[Android Studio releases](https://developer.android.com/studio/releases)
and [AGP 9.2 compatibility](https://developer.android.com/build/releases/agp-9-2-0-release-notes)

### Web

Use **Python 3**, LLVM/Clang with WebAssembly support, and a **thread-enabled
WASI sysroot**. The tested setup is **LLVM 23.1.1 + WASI SDK 34's sysroot** with
matching compiler-builtins. This port does **not** use Emscripten; the versions
above describe the tested combination, not a confirmed minimum.

Set `LLVM_ROOT` to the LLVM installation and `WASI_SYSROOT` to the WASI sysroot,
then run `reflex build web debug` from the project folder. The generated
`projects/web/run_local_debug` and `run_local_release` scripts serve the app
with the required browser headers. Production hosting needs HTTPS and
cross-origin isolation headers: `Cross-Origin-Opener-Policy: same-origin` and
`Cross-Origin-Embedder-Policy: require-corp`.

[LLVM releases](https://github.com/llvm/llvm-project/releases) and
[WASI SDK](https://github.com/WebAssembly/wasi-sdk)

<a id="reflex-build-linux"></a>

### Linux

Use a C++20-capable GCC/G++ toolchain, GNU Make 4.3 or newer, and `pkg-config`.
Generated Linux projects use `g++` and `ar` by default.
Ubuntu 24.04 is the baseline used by the repository's Linux build environment;
minimum compiler and distribution versions have not yet been finalized.

The stock generated UI projects use X11 and require an EGL/OpenGL ES 3
implementation. Audio applications use ALSA. File open/save/folder dialogs
require a working session D-Bus service and `xdg-desktop-portal` with a suitable
desktop backend. Opening URLs, files, and folders through `System::Open`
requires `xdg-open` from the `xdg-utils` runtime package. WSLg alone does not guarantee a working file portal.

#### Debian/Ubuntu packages

Install these development packages for the stock X11 and audio-app targets:

```sh
sudo apt update
sudo apt install -y build-essential cmake pkg-config libcurl4-openssl-dev \
    libegl1-mesa-dev libgles2-mesa-dev libxkbcommon-dev \
    libx11-dev libx11-xcb-dev libxcb1-dev libxcb-util-dev libxcb-cursor-dev \
    libxcb-icccm4-dev libxcb-randr0-dev libxcb-xkb-dev libxcb-xtest0-dev libxcb-sync-dev \
    libxkbcommon-x11-dev libasound2-dev libdbus-1-dev xdg-utils
```

Console-only projects need only the compiler/build tools and the dependencies
their targets declare, including libcurl for the Reflex console target.

#### Building from Windows with WSL

Install WSL 2 from an administrator PowerShell terminal:

```powershell
wsl --install -d Ubuntu-24.04
wsl --set-default Ubuntu-24.04
wsl --list --verbose
```

Launch Ubuntu once to finish account setup; install the packages above inside
it. Confirm WSL version 2 and use the actual distribution name if different.
From PowerShell in the project folder, with Windows `reflex` on `PATH`:

```powershell
reflex generate linux
reflex build linux debug
```

The CLI invokes Make directly in the default WSL distribution; tools must be
available without shell startup scripts. Keep the project and SDK accessible
under standard drive mounts (`D:/dev` becomes `/mnt/d/dev`). Builds produce
Linux binaries. WSLg can run GUI apps; use a native desktop for final desktop
integration checks.

[Microsoft WSL installation guide](https://learn.microsoft.com/en-us/windows/wsl/install)
and [Linux GUI applications with WSL](https://learn.microsoft.com/en-us/windows/wsl/tutorials/gui-apps)


## VS Code

Generation writes a disposable `.vscode/` folder beside `project.cfg` by
default; open that folder's parent in VS Code. Install the Microsoft C/C++
extension (`ms-vscode.cpptools`). Tasks invoke `reflex build`: `<platform>: <target>` builds one target
in Debug, and `<platform>: Release` builds all targets in Release. All build task
labels include the platform name. Tasks require the toolchains
above. Native app debugging uses the Windows debugger, CodeLLDB (`vadimcn.vscode-lldb`) on macOS, or
GDB on Linux (`/usr/bin/gdb`); Linux IntelliSense assumes `/usr/bin/g++`.
Custom compiler paths need matching editor settings. Android, iOS, and Web
have build tasks but no generated deployment/debugging configuration;
Android/Web IntelliSense is not configured. Windows-generated files do not
provide Linux debugging through WSL. CMake Tools (`ms-vscode.cmake-tools`) is unnecessary for this
workflow; generated settings disable its automatic configuration.

## CMake

`reflex generate cmake` emits a root `CMakeLists.txt` for **Windows, macOS, Linux,
and iOS** targets. It requires CMake 3.22 or newer and a compiler/SDK for the
selected target. Android and Web are not emitted by the current CMake
generator; use the Reflex Build workflow for those platforms. Configure and
build CMake output directly with CMake.

| Target | Toolchain and generator |
|---|---|
| macOS | Apple Clang + macOS SDK from Xcode or Command Line Tools; Ninja or Unix Makefiles. The Xcode generator requires full Xcode. |
| iOS | Full Xcode + iOS SDK and **`-G Xcode`**; other generators are unsupported by the SDK library layout. Device builds need signing. |
| Windows | MSVC + Windows SDK from Visual Studio or Build Tools; Visual Studio or Ninja generator. Ninja builds need an MSVC developer shell. |
| Linux | C++20 GCC/G++ or Clang, `pkg-config`, and [development packages above](#reflex-build-linux); Ninja or Unix Makefiles. |

Install CMake and the selected build tool separately where needed; Apple's
Command Line Tools do not supply CMake or Ninja. The shared Reflex CMake
package also supports Wayland via `-DREFLEX_LINUX_BACKEND=wayland`; it needs
Wayland development packages in place of the X11 package set. Stock generated
Linux target settings currently select X11.
