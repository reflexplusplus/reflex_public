# Breaking Changes

## Pending

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
