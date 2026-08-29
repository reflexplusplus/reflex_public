# Changelog

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
