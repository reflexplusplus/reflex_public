# Reflex Agent Notes


## Intro

- Use `include/` as the primary reference surface for API intent, type semantics, and expected usage. This remains the canonical first stop.
- Use `src/reflex_ext/` and `examples/` to see real, working usage examples. Prefer copying patterns from there over inventing usage from headers alone.
- Avoid reading `src/reflex/` unless strictly necessary, as it is primarily low-level implementation detail.
- The generated manual under [documentation/reflex_cpp/info.txt](/D:/devt/reflex/documentation/reflex_cpp/info.txt) is also a useful reference index. Symbols are organized by `Path`, for example:
  - `Path: ["Reflex", "Object"]`
  - `Path: ["Reflex", "TRef"]`
  - `Path: ["Reflex", "Reference"]`
- When orienting in the codebase, prefer looking up symbols by their documented `Path`, then confirming the corresponding header in `include/`, then checking `src/reflex_ext/` for concrete usage.


## Namespace Guide

- `Reflex`: Core framework layer. Common types, containers, intrusive object model, allocation, references, utilities, and shared conventions live here.
- `Reflex::Data`: Structured data, `PropertySet`, serialization/encoding, hashing, and format conversion. Used broadly across the framework for schema-light, portable data.
- `Reflex::File`: Path utilities, file I/O, virtualized/shared resource access, and file-backed resource infrastructure. This is the normal file/resource API for application code.
- `Reflex::GLX`: UI, layout, styling, events, widgets, animation, and rendering-oriented object composition.
- `Reflex::System`: Low-level OS-facing abstraction layer. Files, windows, rendering backends, app/platform services, and native integration points live here. Prefer higher-level namespaces when possible.
- `Reflex::Bootstrap`: Standard application bootstrapping, global app state, and common app/plugin integration glue.
- `Reflex::Async`: Higher-level async helpers built on lower-level system primitives.


## Setup

- To install the pre-built binaries, run the `install` script at the repo root
- Users with access to the source tree, can optionally build the libraries and tools locally via the appropriate platform build scripts under: `build/lib/[platform]` and `build/tools/[platform]`


## Creating Projects

- When creating a new Reflex app or test project, use the reflex CLI instead of attempting to manually set up projects.
- Use `reflex create --template <id> --vendor <vendor> --product <product> --target <id[,..]> --output <folder>`.
- Check available templates with `reflex templates`.
- Check available targets with `reflex targets`.


## Fundamentals

### `Reflex::Object`

- `Reflex::Object` is the common base class for the framework object model.
- It is dynamically typed and retain/release based.
- It has built-in intrusive lifetime management via `Retain*` / `Release*`.
- In normal code, avoid calling `Retain*` / `Release*` manually. Prefer `Reference`, `TRef`, and helper APIs such as `Make<T>()` / `New<T>()`.
- Objects are generally created with `New<T>(...)`, which enables automatic destruction when retain count reaches zero.
- `New<T>(...)` returns a `TRef<T>`, not a `Reference<T>`.
- `Make<T>(...)` is the convenience wrapper that returns a strong owning `Reference<T>` instead of `TRef<T>`.
- `New<T>(...)` uses forwarding, so there are cases where argument deduction cannot resolve the call cleanly, especially with untyped brace arguments such as `{}`.
- In those cases, prefer `REFLEX_CREATE(TYPE, ...)`.
- `Object` also provides the generic object-property mechanism used widely across Reflex and GLX.

### `Reflex::TRef`

- `TRef<T>` is one of the 2 primary Reflex smart-pointer/reference types, and it is critical to understand that it is **not** the owning one.
- `TRef<T>` is a lightweight, non-owning, non-null reference wrapper over a retainable object pointer.
- `T` denotes transitory or temporary, it is generally used for transient arguments and return values, not for long-lived storage.
- `TRef<T>` does **not** affect retain count.
- Think of `TRef<T>` as a raw pointer with stronger semantics:
  - the pointer is logically non-null
  - the target is a Reflex retainable object
  - callers must retain or store into an owning reference type if they need to guarantee lifetime
- `TRef<T>` is therefore commonly used:
  - as the return type of factory-style `Create` / `New` APIs
  - as a function argument when the API contract implies the receiver may retain it if needed
- Do not describe `TRef` as owning.
- Do not infer that returning `TRef` extends lifetime.
- `TRef` is pointer-sized and is intended to be cheap to pass around.

### `Reflex::Reference`

- `Reference<T>` is the other primary Reflex smart-pointer type, and it is the strong owning/retaining one.
- `Reference<T>` increments retain count on construction/assignment and decrements it on destruction/clear.
- Unlike COM-style conventions, Reflex objects are created with a retain count of `0`, so `Reference<T>` has straightforward paired retain/release behavior.
- When code needs to keep an object alive beyond an immediate expression or callback, `Reference<T>` is typically the right tool.
- If you need to turn a `TRef<T>` into an owning reference, use `Reference<T>`.

### `WeakRef`

- `Reflex::Detail::WeakRef<T>` is the weak-reference mechanism for retainable Reflex objects.
- `WeakRef` should be used sparingly.
- In client-side application code, `WeakRef` is almost never needed.
- `WeakRef` is typically a library/framework-internals tool for event systems, object graphs, and lifecycle-sensitive infrastructure.
- Generally, prefer `Reference` for retained ownership.


## Repeated Misunderstandings

- `TRef` is non-owning. `Reference` is owning/retaining.
- `include/` is the best first source of truth for meaning; `src/reflex_ext/` is usually the best place to confirm intended real-world usage.
- `src/reflex/` often explains how something works internally, but should not be the default source for deciding how higher-level code is meant to use an API.
- If a symbol is unclear, search the generated manual by documented `Path` and then inspect the corresponding header.


## Debugging

### Logging

- In debug builds, Reflex writes debug log output to `reflex_log.txt` in the project folder.
- If memory leaks are detected, they are written separately to `reflex_leaks.txt`.
- Check these files when investigating runtime warnings or errors, including issues such as stylesheet compilation failures.

### Visual Verification

- For automated window capture and saving to PNG for verification, see the docs for `Reflex::GLX::CaptureWindow`
