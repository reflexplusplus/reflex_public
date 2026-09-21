# Changelog

## v0.5.2 — CMake include paths and bundle metadata

Reflex 0.5.2 is a maintenance release aimed at how projects consume the SDK. Generated CMake targets now expose their public include directories as PUBLIC, with paths relative to the package, so dependent targets inherit them and exported trees no longer carry absolute paths. Generated macOS and iOS bundles take their minimum OS version from the project's deployment target rather than a fixed value. It also fixes a crash when cloning a null table and an abort that could not proceed while a slow network was being emulated.

### Changes
#### CMake
- The generated `Info.plist` now declares the minimum OS version, taken from the deployment target the `reflex_add_*` helpers already resolve: `LSMinimumSystemVersion` on macOS and `MinimumOSVersion` on iOS. The `reflex build-plist` tool gained the matching `--min_macos <version>` and `--min_ios <version>` arguments; passing neither writes no key.

#### other
- Validate release notes before building a release
- Load the agent notes in Claude Code
- Async::Detail::Fetch dont block abort when emulating slow network
- examples\Custom Drawing remove unsused var
- fix test-projects/minimal
- fix File::Table::Clone would crash if cloning null table
- Expand windows test matrix coverage, add Minimal option
- emit public_include_directories as PUBLIC, with relative paths (#196)

## v0.5.1 — graphics fixes and control state aliases

Reflex 0.5.1 is a maintenance release. It fixes two GLX rendering bugs, separates the bitmap conversion APIs, accepts readable state names on generic controls, and stops spawned processes opening a console window on Windows by default.

### Changes

#### Graphics

- Fixed grayscale JPG metadata handling, and separated the bitmap conversion APIs.
- Fixed a scale and float alignment offset bug.

#### Controls

- `Bootstrap::GenericControl` now accepts the `rotary`, `dragedit`, `popup` and `button` state alias names.

#### System

- `System::Process::Options::allow_window` now defaults to false. Windows only; a spawned process no longer opens a console window unless it asks for one.

#### Bootstrap

- Moved the `Bootstrap::CLI` `MakeTask` helper to the header.

## v0.5.0 — semantic references and audio plugin parameter fixes

Reflex 0.5.0 introduces semantic reference aliases that describe what a reference does with ownership, and renames several persistence and interface APIs to match. It also fixes a number of audio plugin parameter and host-compatibility problems, hardens CoreAudio and WinMM device handling, and moves Android builds to static archives.

### Changes

#### Object model and references

- Added semantic reference aliases — `AlreadyRetained`, `WillRetain`, `Unretained` and their const forms — so a signature states its ownership contract rather than leaving it to convention. They are aliases of `TRef` and `Ref`, so existing code continues to compile.
- Added the `ConstRetained` alias.
- `Reflex::Detail::Initialiser` now supports deferred definition.
- Added `Async::Threadpool`.
- Added a `Reflex::Rotate` array helper.

#### Renamed and removed APIs

- Renamed `Data::iStreamable` to `Data::iSerializable`, and `Bootstrap::Streamable` to `Bootstrap::PersistentState`.
- Deprecated `InterfaceOf` in favour of simpler free functions.
- Removed the deprecated `System::Task::Active`.
- Renamed `include/reflex/system/entry/instance.h` to `app.h`.
- Serialization now prefers a type's custom implementation over a raw copy.
- Added `System::AudioPlugin::GetClass`.

#### Audio plugins

- Fixed a VST3 crash in Ableton Live 12 caused by reporting a state change immediately from within Netx.
- Fixed CLAP `value_to_text` round-tripping.
- Fixed `Bootstrap::ParameterControl` update handling, including a missing `UnsetState`.
- Added `Bootstrap::AudioPlugin::UpdateParameterValue` and a private `SetParameterValueMt`.
- Optimised `Bootstrap::GenericControl` styling.
- Plugin windows now use a universal resize handle.
- Fixed executable names in generated Apple property lists.

#### Audio and MIDI devices

- Guarded the CoreAudio lifecycle when no device is available, and hardened resume re-entry.
- Windows MIDI ports now stay alive when a WinMM open fails or a port is re-enabled, and MIDI hotplug was fixed.

#### Graphics

- Fixed the GLX Tile layer.
- Fixed a missing `glActiveTexture` binding.
- Fixed a macOS `window.mm` compile error.

#### Build, CMake and the CLI

- Android libraries are now generated as static archives instead of AARs.
- Fixed Android CMake target collisions for source dependencies in generated projects.
- `reflex_add_*` targets now honour `CMAKE_OSX_DEPLOYMENT_TARGET` and expose `APPLE_DEPLOYMENT_TARGET` (#179).
- `reflex install --platforms` now takes `windows` rather than `win`, for consistency with other commands.
- Added header files to the Reflex project definition.
- Added a CMake test matrix, and stopped the International Meeting Planner example from failing it on an HTTP error.
- Assorted Reflex CLI fixes and enhancements, including building with current Visual Studio installations.

#### Scripting

- The VM now converts `bool` to `int32` where it previously did not.

## v0.4.4 — more flexible project generation

Reflex 0.4.4 expands the project generator with reusable dependency packages and host-appropriate defaults. It also improves generated CMake apps, installation guidance, and automated agent testing.

### Changes

#### Reflex Build and project generation

- Added `@Package` declarations for named, reusable bundles of target, library, and package dependencies.
- `reflex generate` and `reflex create` now choose platforms compatible with the current host when none are specified; CMake generation remains opt-in.
- Fixed the CMake source path in generated C++ app projects.

#### Tooling and guidance

- `--auto-quit` now accepts whole-second values, making automated application runs easier to configure.
- Updated the public bootstrap instructions to accept a chosen installation directory directly.

## v0.4.3 — clearer CLI validation and guidance

Reflex 0.4.3 improves installation guidance for developers and AI agents, and makes ReflexCLI report invalid command arguments clearly before running a command.

### Changes

#### Documentation

- Clarified where the bootstrap scripts install the public Reflex++ source folder.
- Added a reminder for AI agents to read `AGENTS.md` after installing the SDK.

#### ReflexCLI

- Commands now validate their arguments before execution, with clear errors for unknown, excess, and missing arguments.

## v0.4.1 — portable builds and audio-plugin parameters

Reflex 0.4.1 makes generated desktop and Android builds more portable, with consistent target defaults, dependency handling, and path resolution across the native and CMake workflows. It also modernizes the Bootstrap audio-plugin parameter API, so existing audio plugins should review the accompanying migration guidance before updating.

### Changes
#### Project generation and CMake

- Centralized generated-target creation and platform defaults, so native and CMake projects use the same baseline configuration.
- Improved handling of relative paths and task imports; `$(TASK_FILE_DIRECTORY)` is available when importing task files, and persisted paths are normalized consistently.
- Added Android AAR dependencies to generated projects and made generated plist output create its destination folder when necessary.
- Added registration for external libraries, making them available to generated project configurations.

#### Android builds and signing

- Android debug and release signing are now configurable through generated project settings.
- Builds continue when an optional signing file is unavailable, allowing unsigned development builds without local signing material.
- Refined Android templates and build sources to remove warnings.

#### Audio-plugin API migration

- Reworked Bootstrap audio-plugin parameters around abstract `ParameterDefinition` types and factory functions for continuous, discrete, enum, and boolean parameters.
- Replaced the older parameter-control construction path with controls that bind directly to an audio-plugin instance and parameter index.
- See [BREAKING_CHANGES.md](../../BREAKING_CHANGES.md) for the required source migrations.

#### Stability and delivery

- Fixed a crash in the GLX `If` layer when its object has zero size.
- Refined Bootstrap controls and module initialization naming.
- Pre-release tags no longer trigger the public-repository export workflow.

## v0.3.30

### Bootstrap multi-class audio plugins

- Bootstrap audio-plugin binaries can now publish multiple classes and select the requested class for instance creation, parameter registration, AUv2, and AUv3.
- Audio Unit configuration now uses comma-separated `AU_COMPONENTS` records, allowing several component registrations to share one binary.
- This changes the Bootstrap source interface even for single-class plugins. Existing plugins must update `MakeClass`/`Create` and constructor signatures while preserving their published plugin identifiers. See [BREAKING_CHANGES.md](BREAKING_CHANGES.md#bootstrap-audio-plugins-now-register-a-list-of-classes) for the migration guide.

### Ownership diagnostics

- Added `[[nodiscard]]` to `Reflex::New` and selected object-creating/acquiring APIs. Existing user code that intentionally ignores these results may now produce compiler warnings.

## v0.3.12

### Data table and documentation tooling

- Added `Reflex::Data::Table` as a first-class `Reflex::Data` API, including typed in-memory table storage plus query, filter, sort, slice, group, count, and aggregate helpers.
- Added a new `reflex doc` documentation mode to `ReflexCLI`.
- Added template-local `AGENTS.md` files and refreshed the root agent guidance.

## v0.3.10

### GLX image and styling improvements

- Added `GLX::BilinearResizeBitmap` and moved bitmap helper functionality into `reflex_ext`.
- Added indent support for the `ImageMask` style layer.
- Improved GLX stylesheet conditional-variable error reporting and related documentation coverage.
- Renamed `GLX::Detail::ClearImage` to `UnsetImage`.

## v0.3.8

### IDE and logging cleanup

- Simplified the IDE/bootstrap logging path so file logging is configured through Bootstrap preferences rather than being passed into `IDE::Start(...)` directly.
- Added a Bootstrap settings-panel option for file logging mode (`Off`, `Auto`, `Always`), including a debug-friendly auto mode that writes `reflex_log.txt` into the project folder.
- Cleaned up warning-scope handling and debug log formatting so queued, console, and file output share the same formatted message shape more consistently.

### Documentation and template hygiene

- Expanded the Reflex C++ manual entries, including additional core helper discoverability and ownership/reference notes.
- Added `reflex_log.txt` to relevant template/example/test-project `.gitignore` files and added missing `.gitignore` coverage for several tool build folders.

## v0.2.02

### GLX multi-touch support

- GLX pointer input now supports optional multi-touch delivery. The `kMouseXXX` event IDs are aliases of the `kPointerXXX` IDs, and `GetClickFlags(...)` remains available as an alias over the pointer flags path.
- Added `kPointerFlagMulti` to mark non-primary simultaneous touches, along with `Object::EnableMultiTouch()` to opt objects into receiving second, third, and later touches.
- For implementation guidance and event-handling details, see the `GLX > Events > MultiTouch` guide in the Reflex C++ documentation.

### Bootstrap mobile emulation

- Improved mobile emulation in the Bootstrap main window, including tighter device-size handling for emulated mobile previews.
