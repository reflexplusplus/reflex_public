# Breaking Changes

## v0.7.0

### AAX and VST2 plugins require explicit SDK paths

Building the `AAX` format with `reflex_add_audio_plugin` now requires a separately installed Avid AAX SDK, version 2.9 or later. Set `AAX_SDK_PATH` as a CMake cache variable or an environment variable pointing to the SDK root. CMake builds the SDK once using its supplied CMake project, and each AAX plugin links against it. Configuration fails for an older SDK and issues a warning for a newer version, as only 2.9 has been tested. On macOS, Makefile and Ninja projects that compile their own `.mm` files must enable `OBJCXX`. For projects generated from the `cpp_audioplugin_aax` template, set the SDK path with `reflex set AAX_SDK_PATH <path>`. If you build Reflex from a source checkout, rebuild `ReflexTargetAAX`.

For VST2, the stock cfg definitions for `ReflexTargetVST2` and `cpp_audioplugin_vst2` now require a nonempty `VST2API_PATH`. Set it to the folder containing `vst2api.h` with `reflex set VST2API_PATH <path>`, then regenerate projects that use those definitions.

### `GLX::AbstractViewPort` bars use type descriptors

Scrollbar positioning can now be configured in the viewport stylesheet's `x` and `y` blocks, including the new `float | float` positioning mode. For example:

```text
x:
{
    inherit: bar;
    positioning: float;
};

y:
{
    inherit: bar;
    positioning: inline;
};
```

`GLX::AbstractViewPort::ViewBarCtr` and `SetViewBarCtr(bool y, ViewBarCtr)` have been replaced by `ViewBarType` and `SetViewBarTypes(ViewBarType x, ViewBarType y)`. Configure both axes in one call.

Replace `kNullBar` with `kPlaceholderBar` when a placeholder of a specified size is intended. Use `EnableViewBars(bool x, bool y)` to toggle bars on and off independently of their type.

For example, to configure a horizontal scrollbar and a vertical paginator:

```cpp
viewport.SetViewBarTypes(GLX::AbstractViewPort::kScrollBar, GLX::Paginator::kType);
```

### Plugin host names are queried per instance

The static `System::AudioPlugin::GetPluginHost()` has been removed. Call `GetHostName()` on the plugin instance instead. It returns an owning `CString`, which may be empty for formats that do not report a host; VST3 only knows the name after the host initializes the instance. Custom implementations of `System::AudioPlugin` must implement the new pure virtual method `CString GetHostName() const`.

### Process options and lifetime API

- The deprecated `System::Process::Detach()` has been removed.
- On Windows, `Process::Options::allow_window` now defaults to `true`, allowing console inheritance. Set it explicitly to `false` to retain the previous window-suppression behavior. That mode uses `CREATE_NO_WINDOW` and cannot redirect to console handles.
- `Options` now has a `FileHandle * std_err` member between `std_out` and `allow_window`.

### Project configuration and generation

- The framework build description has moved from `build/lib/project.cfg` to `project.cfg` at the repository root.
- `OutputDirectory` and `Entitlements` are now reserved project variables derived from target properties. Remove custom `@Variables` declarations with those names and use the `output_directory` and `entitlements` properties instead.
- `generated_directory` is now read only from the root project document, rather than inherited from includes. Declare it in the root document if needed; it may name a nested folder but must resolve inside the project directory.

### Await callbacks run after their registration is removed

`Async::AttachAwait` now removes the completed await registration before invoking its callback. `Async::IsAwaiting(object, id)` therefore returns `false` inside the completion callback unless a new await has been attached. The callback can safely attach another await under the same ID; completion cleanup no longer removes the replacement.


## v0.6.0

### `Output::SetLogFile` and `Output::Disable` replaced by `Output::Enable`

`Reflex::Output::Enable(bool console, WillRetain<System::FileHandle> logfile)` sets both output targets in one call. `console` enables or disables console output and log capture to the queue; a non-null `logfile` enables file output, and a null one disables it.

```cpp
// Before
Output::SetLogFile(file);
Output::Disable();

// After
Output::Enable(true, file);
Output::Enable(REFLEX_DEBUG, {});
```

`Disable()` kept console output in debug builds; pass `REFLEX_DEBUG` as `console` for the same result, or `false` to silence all output.

## v0.4.1

### Bootstrap audio-plugin parameters use abstract definitions

`Bootstrap::ParamDesc` is now `Bootstrap::ParameterDefinition`. A definition contains all parameter metadata; there is no separate description object. `ParameterDefinition` is abstract, and Bootstrap's continuous, discrete, enum, and boolean definitions are created through the free factory functions.

Update parameter registration to use the free factory functions and pass the parameter group mask when the definition is created:

```cpp
// Before
auto desc = Bootstrap::ParamDesc::CreateReal(
    "Gain", 0.0f, 1.0f, 0.0f, 0.5f);
desc->change_flags = kParameterGroupGain;

// After
auto definition = Bootstrap::DefineContinuousParameter(
    L"Gain", 0.0f, 1.0f, 0.001f, 0.5f,
    kParameterGroupGain);
```

`DefineContinuousParameter(...)` takes `step` where `CreateReal(...)` previously took `origin`; `initial` becomes the control's default/reset value. Its optional final argument is a `FunctionPointer<WString(Value32)>` used to format the value. The remaining standard factories are `DefineDiscreteParameter`, `DefineBoolParameter`, and `DefineEnumParameter`.

The deprecated static compatibility factory remains named `ParameterDefinition::CreateReal(...)` and forwards to `DefineContinuousParameter(...)`.

The old writable `to_string` member is gone. Continuous parameters can supply a formatting callback to `DefineContinuousParameter(...)`. For custom parsing, or behaviour beyond the standard continuous, discrete, enum, and boolean definitions, derive from `ParameterDefinition` and implement its metadata, `ToString(...)`, and `FromString(...)` virtual methods.

For direct metadata access, migrate as follows:

- `name` -> `GetName()`
- `init_value` -> `GetDefaultValue()`
- `change_flags` -> `GetGroupFlags()`
- `flags` -> `GetFlags()`
- `type` -> `GetType()`
- `min` and `max` -> `GetRange().a` and `GetRange().b`
- `origin` -> `GetStep()`
- `to_string(value)` -> `ToString(value)`; parsing is `FromString(value)`
- `kTypeReal`, `kTypeEnum`, and `kTypeBool` -> `kTypeContinuous`, `kTypeEnumeration`, and `kTypeBoolean`

`PopulateParameters(...)` should now receive `ConstReference<Bootstrap::ParameterDefinition>` entries. The second `OnProcessRt(...)` argument is the combined parameter group mask; renaming it from `parameter_change_flags` to `parameter_group_flags` is recommended for clarity but does not change the override signature.

`ParamControl::Create(parameter, value)` has been removed. Audio-plugin editors should use `ParamControl::Create(instance, parameter_index)`, which binds the control directly to the plugin and handles automation internally. Custom parameter controls can derive from `Bootstrap::ParameterControl` and bind through `ParameterControl::ParameterInterface`.

## v0.3.30

### Bootstrap audio plugins now register a list of classes

The Bootstrap audio-plugin contract now supports multiple plugin classes in one binary. This is a source compatibility break even for plugins that publish only one class.

```cpp
// Before
static Class MakeClass();
static TRef<Instance> Create(System::AudioPlugin & owner);

InstanceImpl(System::AudioPlugin & owner)
    : Instance(owner, kMagic, kChunkVersion)
{
}

// After
static Array<Class> MakeClasses();
static TRef<Instance> Create(
    const Class & cls,
    System::AudioPlugin & owner);

InstanceImpl(const Class & cls, System::AudioPlugin & owner)
    : Instance(cls, owner, kMagic, kChunkVersion)
{
}
```

Return the old class in a one-element array, update `Create` to accept the selected `cls`, and pass that same class to the `Bootstrap::AudioPlugin` base constructor. Bootstrap now calls `PopulateParameters(...)` once per class.

Preserve every identifier used by an already released plugin. In particular, every class now requires a stable, unique `clap.uid` even for non-CLAP builds because Bootstrap uses it to associate parameter definitions with the selected class. Changing CLAP, VST2, VST3, or Audio Unit identifiers can make hosts treat the update as a different plugin.

Audio Unit projects now declare one or more components through `AU_COMPONENTS`, using comma-separated `subtype:type[:name]` records. `AU_VENDOR_4CC` is the shared manufacturer identifier.

For CMake, replace:

```cmake
AU_TYPE_4CC   "aufx"
AU_UID_4CC    "MyFx"
AU_VENDOR_4CC "MyCo"
```

with:

```cmake
AU_COMPONENTS "MyFx:aufx"
AU_VENDOR_4CC "MyCo"
```

Use the same values in `project.cfg`, with property syntax, and run `reflex generate --path <project.cfg>` after changing them.

For the complete registration, identifier-stability, parameter, and multi-class Audio Unit requirements, run `reflex doc info Reflex::Bootstrap::AudioPlugin --root cpp`.

## v0.2.02

### Pointer capture now defaults to linked drag/up after trapped `kMouseDown`

If an object traps `kMouseDown`, it now receives linked `kMouseDrag` / `kMouseUp` by default.

Previously, many call sites used `EnableMouseCapture(...)` only to opt into drag/up delivery. That opt-in is no longer needed, so migration is not a mechanical rename to `EnablePointerCapture(...)`.

When updating existing code:

- remove the old capture call entirely if it only existed to enable normal drag/up delivery
- use `EnablePointerCapture(e, false)` on press paths that should remain click-only
- use `EnablePointerCapture(e, true, true)` only for sites that require incremental capture

Review conditional press-path handlers carefully, especially:

- RMB vs LMB flows
- modifier-dependent modes
- early-return or no-op click paths
- any `kMouseDrag` logic that assumes `kMouseDown` initialized drag state

### `GetMousePosition(...)` and `GetMouseDelta(...)` are deprecated

GLX pointer helpers now use pointer-oriented names and event-carried position data.

When updating existing code:

- replace `GetMousePosition(...)` with `GetPointerPosition(...)`
- replace `GetMouseDelta(...)` with `GetDelta(...)`

See documentation/glx-pointer-events-migration-guide.md


## v0.2.004

### Bootstrap::App inherits Streamable

Your App impl no longer needs to inherit from Streamable, after updating, pass your chunkversion to the constructor.

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
