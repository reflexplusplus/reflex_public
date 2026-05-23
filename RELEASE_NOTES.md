# Release Notes

## Pending

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
