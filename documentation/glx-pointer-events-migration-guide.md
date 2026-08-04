**GLX Pointer Events Migration Guide** *(ref: `d0b008634`)*

## What changed

Input is now pointer-based, with full support for multi-touch via parallel slot-based captures. Key shifts:
- `kMouseDown/Drag/Up` remain as aliases for `kPointerDown/Drag/Up` — existing handlers keep working
- Pointer events carry richer data: slot, timestamp, position, capture
- Object input config, capture model, and position helpers all have new APIs

---

## Key API Changes at a Glance

| Old | New | Notes |
|-----|-----|-------|
| `EnableMouseCapture(obj, en, incr)` | `EnablePointerCapture(e, en, incr)` | intent moves to event; see below |
| `GetMousePosition(obj)` / `window.GetMousePosition()` | `GetPointerPosition(obj, e)` or `GetPointerPosition(window)` | `WindowClient::GetMousePosition()` was removed |
| `GetMouseDelta(e)` | `GetDelta(e)` | blanket replace |
| `SetMouseOverTrapMode(...)` / `SetMouseClickTrapMode(...)` | `EnablePointer(enabled, active)` | — |
| `kClickFlagRmb` / `kClickFlagDbl` | `kPointerFlagRightMouseButton` / `kPointerFlagDouble` | old names still compile |

---

## Capture Migration (most complex — read carefully)

**Semantic shift:** objects that trap `kPointerDown` now receive linked drag/up **by default**.

This is **not** a mechanical rename. For each `EnableMouseCapture(...)` call site:

**→ Delete it** if it only existed to get linked drag/up after down. That's now the default.

**→ Replace with `EnablePointerCapture(e, false)`** when some down paths should *not* enter a drag sequence (RMB, modifier-key modes, paths with no drag-state init):
```cpp
case kPointerDown:
    if (rightClick)
        EnablePointerCapture(e, false); // opt out of drag/up
    else
        { /* init drag state */ }
    return true;
```

**→ Replace with `EnablePointerCapture(e, true, true)`** only when the old code relied on incremental capture.

**High-risk sites:** handlers where drag state is conditionally initialized, RMB/LMB differ, modifier keys switch modes, or `kPointerDrag` assumes `kPointerDown` always ran setup.

---

## Position Migration

**In pointer event handlers** (`kPointerDown/Drag/Up`, `kMouseWheel`, `kDragDropTender`):
```cpp
auto pos = GLX::GetPointerPosition(*this, e);
```

**Ambient / no event available** (hover, autoscroll, popups):
```cpp
auto pos = GLX::TransformPosition(*this, GLX::GetPointerPosition(GetWindow()));
```

**If you previously used `WindowClient::GetMousePosition()` directly:**
```cpp
auto pos = GLX::GetPointerPosition(window);
```

> ⚠️ `GetPointerPosition(obj, e)` only works on pointer-bearing events. Don't call it on key, focus, transaction, or custom events.

---

## Multi-Touch

Multi-touch is a new capability in this model — no migration required, but existing handlers should be aware of it.

Each simultaneous touch contact is assigned a **slot** — an index that stays stable across the down/drag/up sequence for that finger. This allows objects to track multiple independent pointer sessions in parallel, each with its own capture state.

Relevant APIs:

- `GetPointerSlot(e)` — returns the slot index for this event; use it as a key when storing per-touch state
- `kPointerFlagMulti` — set on events that are part of a multi-touch sequence; lets handlers distinguish single-touch from multi-touch if needed
- `GetTimestamp(e)` — returns the OS-provided precise timestamp for the event; under the hood, GLX now propagates native per-touch timestamps rather than a coarse frame time, which matters for velocity and gesture timing

To opt an object into receiving multi-touch events:
```cpp
EnableMultiTouch(true);
```
Without this, objects receive only the first active touch and can ignore slot handling entirely.

---

## Checklist

- [ ] `EnableMouseCapture(...)` → delete or replace per rules above
- [ ] `GetMousePosition(...)` → `GetPointerPosition(...)`
- [ ] `GetMouseDelta(e)` → `GetDelta(e)`
- [ ] `SetMouse*TrapMode(...)` → `EnablePointer(...)`
- [ ] `kClickFlag*` → `kPointerFlag*` (optional, old names still compile)
- [ ] Tap-or-X gesture factories → explicit gesture constructors
