# Breaking Changes


## v0.2.004

### Bootstrap::App inherits Streamable

Your App impl no longer needs to inherit from Streamable, after updating, pass your chunkversion to the constructor.

### AudioUnit 4CC token names renamed across CMake and macOS template surface

AudioUnit token names were aligned so the public CMake API and the macOS plist/Xcode template surface now use explicit `_4CC` suffixes and the clearer `VENDOR` terminology.

When updating an existing CMake-based AudioUnit project, make these replacements:

- `AU_TYPE` -> `AU_TYPE_4CC`
- `AU_SUBTYPE` -> `AU_UID_4CC`
- `AU_MANUFACTURER` -> `AU_VENDOR_4CC`

When updating generated macOS template/Xcode AudioUnit settings or plist substitutions, make this replacement:

- `AU_COMPANY_4CC` -> `AU_VENDOR_4CC`

The underlying values are unchanged: these are naming updates only. The important requirement remains that each AudioUnit 4CC token must still be a 4-character code.

### `System::FileHandle::Flush` now requires a `bool commit` argument

`System::FileHandle::Flush()` changed signature to `bool Flush(bool commit)` in [include/reflex/system/file_handle.h](/D:/devt/reflex/include/reflex/system/file_handle.h:81).

This is a source compatibility break for callers that previously invoked the no-argument form. Existing code now needs to choose whether it wants a lightweight flush or a durable commit flush:

- `Flush(false)` for a non-committing flush boundary
- `Flush(true)` for a commit-style flush used by persistence save paths

### `System::FileHandle::Status()` removed

`System::FileHandle::Status()` was removed from the file-handle interface.

For the concrete file-handle implementations, `Status()` had effectively collapsed to "is this a real handle or the null handle", so callsites that only needed that validity check should now use `IsValid(fileHandleRef)` or an equivalent ref-validity test such as `if (fileHandleRef)`.

Callsites that previously used `Status()` as a save-success signal should instead use the explicit operation results:

- ref validity for open/create success
- returned byte counts for read/write success
- `Flush(false)` or `Flush(true)` for flush/commit success

### `Data::SerializableFormat::Deserialize(...)` now reports explicit deserialize errors

`Data::SerializableFormat` now returns a `DeserializeError` enum from `Deserialize(...)`, and the `Decode(...)` path uses that to surface top-level blob validation failures more explicitly.

This is primarily an external-input validation mechanism for `Decode(blob)` style usage. It should not be read as a stronger promise that all nested deserialization in Reflex is now fully corruption-tolerant or recoverable: the lower-level deserialization contract still assumes valid stream ordering and correct handler behavior once execution enters trusted internal type-specific restore code.

### `System::Rename` now follows POSIX replace-existing semantics

`System::Rename(...)` is now documented and implemented as a POSIX-style rename in [include/reflex/system/functions.h](/D:/devt/reflex/include/reflex/system/functions.h:43): it replaces an existing destination and is intended to fail across filesystems or volumes.

This is a behavior/API compatibility change for callers that relied on the previous Windows-specific split:

- `System::Rename(...)` is now the atomic publish primitive used by `Reflex::File::Save(...)`
- code that previously used `System::ReplaceFile(...)` should migrate to `System::Rename(...)`


## v0.2.002

### `GLX::Detail::StandardLayout` constructor no longer takes an owner object

`GLX::Detail::StandardLayout` no longer exposes the previous `StandardLayout(GLX::Object & owner)` constructor in [include/reflex/glx/layout/detail/standard_layout.h](/D:/devt/reflex/include/reflex/glx/layout/detail/standard_layout.h:58).

This is a source compatibility break for downstream code that directly instantiates `StandardLayout` or derives from it using inherited constructors. Code written like this will need to be updated:

- `New<MyLayout>(object)` -> `New<MyLayout>()`
- derived layout types that relied on `using StandardLayout::StandardLayout;` now need to remove that dependency and provide their own constructors only if they actually need one


## v0.1.015

### `DebugOutput` renamed to `Output` (compatibility alias currently retained)

The core debug/logging type was renamed from `DebugOutput` to `Output` in the public headers, for example in [include/reflex/core/debug/output.h](/D:/devt/reflex_minimal/include/reflex/core/debug/output.h:14).

At the moment this is softened by a deprecated compatibility alias:

- `DebugOutput` remains available as a deprecated typedef of `Output`
- dependent declarations such as `File::output`, `GLX::output`, `VM::output`, `VM::client`, `Output::Profiler`, and helper APIs now use the new `Output` name in their primary declarations

This is not expected to be an immediate hard source break for most callers, but downstream code should migrate from `DebugOutput` to `Output` to avoid future breakage when the deprecated alias is eventually removed.

### `Async::CreateHttpRequest` network simulation rename and expansion

The request throttling/testing parameter on `Async::CreateHttpRequest(...)` was renamed from `MaxByteRate` to `NetworkSimulation` in [include/reflex_ext/async/http.h](/D:/devt/reflex_minimal/include/reflex_ext/async/http.h:77).

This is potentially significant for callers because it is not just a cosmetic rename:

- the parameter type changed from `MaxByteRate` to `NetworkSimulation`
- all `kMaxByteRate*` constants were removed
- the new enum now mixes bandwidth simulation values with explicit failure/HTTP-status simulation values such as `kNetworkSimulationNoConnection`, `kNetworkSimulation400`, `kNetworkSimulation401`, `kNetworkSimulation403`, and `kNetworkSimulation404`

Existing code that passes `MaxByteRate`, stores that enum type, or references `kMaxByteRateUnlimited`, `kMaxByteRate384k`, `kMaxByteRate2G3G`, `kMaxByteRateSlowMobile`, `kMaxByteRatePoor4G`, `kMaxByteRateTypicalMobile`, `kMaxByteRateBroadband`, or `kMaxByteRateAlwaysFail` will need to be updated to the corresponding `NetworkSimulation` names.

### `PopupBehaviour::SetConfig` virtual signature change

`PopupBehaviour::SetConfig(...)` gained a third parameter, `Key32 content_style = kmenu`, in [include/reflex_ext/glx/behaviours/popup.h](/D:/devt/reflex_minimal/include/reflex_ext/glx/behaviours/popup.h:33).

This is potentially significant because `SetConfig(...)` is a virtual function. Existing derived classes that override the previous 2-argument signature will no longer match the base declaration and will fail to override until they are updated to accept the new third parameter.

### `kTypeID<T>` removed from public header surface

`kTypeID<T>` was replaced by `GetTypeID<T>()` in the public headers.

This is mainly an implementation-detail cleanup, but downstream code that directly references `kTypeID<T>` will need a mechanical update to `GetTypeID<T>()`.

### Deprecated `SetGraphicLayerOnDraw` / `UnsetGraphicLayerOnDraw` removed

`include/reflex/glx/style/custom.h` was removed, along with the previously deprecated `SetGraphicLayerOnDraw(...)` and `UnsetGraphicLayerOnDraw(...)` declarations.


## v0.1.011

### GLX typed property helper rename

`GLX` typed property helpers were renamed for consistency with `Data::Set/GetFloat32`-style accessors.

When patching existing code, replace the older `*Property` and `*Value` helper names with the new `SetX` / `GetX` / `UnsetX` forms:

- `SetPointProperty` -> `SetPoint`
- `UnsetPointProperty` -> `UnsetPoint`
- `GetPointValue` -> `GetPoint`
- `SetSizeProperty` -> `SetSize`
- `UnsetSizeProperty` -> `UnsetSize`
- `GetSizeValue` -> `GetSize`
- `SetMarginProperty` -> `SetMargin`
- `UnsetMarginProperty` -> `UnsetMargin`
- `SetColourProperty` -> `SetColour`
- `UnsetColourProperty` -> `UnsetColour`
- `GetColourValue` -> `GetColour`
- `SetColorProperty` -> `SetColor`
- `UnsetColorProperty` -> `UnsetColor`
- `GetColorValue` -> `GetColor`

These names are unchanged and still return the underlying property object:

- `GetPointProperty`
- `GetSizeProperty`
- `GetMarginProperty`
- `GetColourProperty`
- `GetColorProperty`

This is a source compatibility change only. The intent is to make GLX property access read like the rest of Reflex `Data` accessors, so value reads are `GetX(...)` and writes are `SetX(...)` instead of mixing `GetXValue(...)` with `SetXProperty(...)`.
