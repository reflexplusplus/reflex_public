# Release Notes

## v0.2.02

### GLX multi-touch support

- GLX pointer input now supports optional multi-touch delivery. The `kMouseXXX` event IDs are aliases of the `kPointerXXX` IDs, and `GetClickFlags(...)` remains available as an alias over the pointer flags path.
- Added `kPointerFlagMulti` to mark non-primary simultaneous touches, along with `Object::EnableMultiTouch()` to opt objects into receiving second, third, and later touches.
- For implementation guidance and event-handling details, see the `GLX > Events > MultiTouch` guide in the Reflex C++ documentation.

### Bootstrap mobile emulation

- Improved mobile emulation in the Bootstrap main window, including tighter device-size handling for emulated mobile previews.


## v0.2.004

### Reflex CLI tooling

- Added the new `reflex` CLI tool as the shared backend for project/template creation and resource compilation.
- `reflex create ...` now instantiates Reflex projects from templates, and `reflex build-resources --path ...` now compiles `resources.xml` inputs into embedded C++ sources.
- `ReflexProjectCreator` and `ReflexResourceBuilder` now act as front ends for this CLI rather than owning separate backend implementations.
- Project build steps and generated projects should invoke `reflex build-resources --path ...` directly instead of calling the old Resource Builder app.

### Space indentation

- `GLX::TextEdit` and the Reflex IDE text editor now support space-based indentation, improving editing behavior in projects that do not use tab indentation. Use the context menu in the IDE text editor to select indentation mode.
- Fixed IDE search behavior.

### Path layer improvements

- The `Path` layer supports join and cap options.

### SVG compound fill support

- Added `GLX::FillRule` and a compound-contour `GLX::AddPolygonFill(...)` overload for fills that contain holes or multiple contours.
- SVG path decoding now respects `fill-rule` for compound `<path>` geometry, fixing icons and vector shapes such as donuts, letterform cutouts, and other hole-containing paths that previously rendered with filled-in interiors.


## v0.2.003

### Persistence and file-save hardening

- `Reflex::File::Save(...)` now writes through a temp file and publishes with `System::Rename(...)` instead of overwriting the destination in place. This narrows interrupted-save failure modes so callers should now keep either the previous complete file or the newly written complete file, rather than exposing a partially overwritten destination file during the normal save flow.
- `System::Rename(const WString & from, const WString & to)` is now the explicit same-filesystem replace-existing publish primitive, and `System::Move(const WString & from, const WString & to)` is available for broader relocations that may cross filesystems or volumes.
- `System::FileHandle::Flush(...)` now explicitly distinguishes between a lightweight flush and a durable commit flush via a required `bool commit` argument, and save paths now use the durable commit form before publishing the final file.
- `System::FileHandle::Status()` is now deprecated for file-save success checks. Callers should prefer null-handle/open checks, full write-count checks, and `Flush(true)`.
- `Data::SerializableFormat` now exposes explicit deserialize error reporting for external blob decode paths. This is primarily intended to let `Decode(...)` validate top-level persisted input more cleanly; it is not a guarantee of fully recoverable or corruption-proof deserialization through every nested type handler, and the lower-level `Deserialize(...)` contract still fundamentally assumes valid stream ordering and trusted internal structure.


## v0.2.002

This release is mostly focused on tooling and packaging improvements around public export and documentation generation, with one functional GLX fix.

### Highlights

- Improved the Reflex documentation/export tooling and refreshed the packaged documentation binaries for Windows and macOS.
- Updated the public export flow and Reflex Exporter handling, including preservation of executable permissions for `.command` scripts in templates/examples.
- Fixed a GLX layout issue where float-layout max constraints were ignored when both axes used fit mode.

### Notes

- Most changes in this version are build, export, packaging, and documentation-tool updates rather than runtime API changes.
- For source-compatibility notes, see [BREAKING_CHANGES.md](/D:/devt/reflex/BREAKING_CHANGES.md).
