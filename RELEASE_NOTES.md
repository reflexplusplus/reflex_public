# Release Notes

## Unreleased

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
