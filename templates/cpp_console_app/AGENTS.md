This is a Reflex++ based project. This file is a copy of the `AGENTS.md` from `_REFLEX-PATH_`.
The Reflex API is available at `_REFLEX-PATH_/include`.

# Reflex++ Agent Notes

## Install

- Use the `reflex` CLI to install and update the SDK and obtain required binary packages: `reflex install [version] --platforms <win|macos|android|ios[,..]>`.
- Use `reflex versions` to list available SDK versions, `reflex version` to check the installed version, and `reflex where` to find its installation directory.
- Source users can build libraries and tools with the platform scripts in `build/lib/[platform]` and `build/tools/[platform]`.

## Finding APIs

- Start with `reflex doc tree` to see the available documentation roots, then run `reflex doc info Reflex` for the framework overview and links to its fundamental guides.
- Follow the relevant guide before searching individual APIs, for example `reflex doc info Reflex/Object`, `reflex doc info Reflex/Containers`, or `reflex doc info Reflex/String`.
- Use `reflex doc info <symbol>` for exact types and functions, for example `reflex doc info Reflex::Reference` or `reflex doc info Reflex::Array`. The shorter `reflex doc <symbol>` form is equivalent for direct symbol lookup.
- If the symbol name is uncertain, use `reflex doc list <query>` to search the documentation index.
- After finding the documented API, confirm declarations and detailed type semantics in `_REFLEX-PATH_/include`, then check `_REFLEX-PATH_/src/reflex_ext/` and `_REFLEX-PATH_/examples/` for established usage patterns.
- !IMPORTANT: The `reflex doc` command stores its selected codebase and tree location in the user's preferences. For consistent results, pass `--root <codebase>` (normally `--root reflex_cpp`) with all queries. This overrides the stored codebase and location.

## Namespace Overview

- `Reflex`: Core object model, containers, allocation, references, and utilities.
- `Reflex::Data`: Structured data, serialization, hashing, and format conversion.
- `Reflex::File`: Paths, file I/O, and resource access; prefer this for application-level file work.
- `Reflex::GLX`: UI, layout, styling, events, widgets, animation, and rendering.
- `Reflex::System`: Low-level platform services, windows, files, and rendering backends; prefer higher-level APIs when available.
- `Reflex::Bootstrap`: Application bootstrapping and common app/plugin integration.
- `Reflex::Async`: Higher-level asynchronous helpers.

## Project Creation

- Create apps and tests with the CLI: `reflex create --template <id> --vendor <vendor> --product <product> --target <id[,..]> --output <folder>`.
- List available options with `reflex templates` and `reflex targets`.

## Object Lifetimes

- Reflex++ distinguishes reference-counted `Object`s from stack-based values. Do not use raw `new` for either: create objects with `New<T>()`, and heap-allocate a value only by promoting it to `ObjectOf<T>`.
- `Reflex::Object` is dynamically typed and uses intrusive retain/release lifetime management. Do not call `Retain*` or `Release*` directly in normal code.
- `TRef<T>` is a non-owning, non-null transient reference. It does not change retain count and does not extend lifetime; use it for temporary arguments and factory-style returns.
- `Reference<T>` is the strong owning reference. Store one whenever an object must outlive the current expression or callback.
- `New<T>(...)` returns `TRef<T>`; `Make<T>(...)` returns `Reference<T>`. If `New` cannot deduce an untyped argument such as `{}`, use `REFLEX_CREATE(TYPE, ...)`.
- `Reflex::Detail::WeakRef<T>` is primarily an internal lifecycle tool; application code should normally use `Reference<T>` instead.

## Common Mistakes

- Match the reference type to ownership: use `Reference<T>` for retained storage. Incorrect ownership can cause leaks or premature destruction; in particular, do not return a newly created object through a `TRef<T>` signature via a temporary `Reference<T>`.
- Do not include individual reflex headers in standard project templates; the framework API is already available.
- Keep `ArrayView` and string views within the lifetime of their source data.
- `ArrayView::size` is not `Array::GetSize`; use the appropriate API for the type in hand.

## Debugging

- Use the project namespace's `output` symbol for logging. If it is unavailable in the current scope, use `Reflex::File::output`.
- Debug builds write runtime output to `reflex_log.txt` and leak reports to `reflex_leaks.txt` in the project folder.
- For automated window screenshots, use `Reflex::GLX::CaptureWindow`; see the documentation for a working example.
- For autonomous assertion debugging, the C++ Console App debug template supports `--terminate-on-assert`. An assertion then flushes its message and any available stack trace to `reflex_log.txt` before terminating with exit code 1, rather than waiting at a debug break.
- See `code/entry.cpp`, `System::OnStart`, for command-line parsing and the installed `OnDebugBreak` handler.

## Resources and Hot Reload

- Template builds run `reflex build-resources` automatically. It compiles `resources.xml` into `code/resources.h` and `code/resources.cpp`; edit the XML and source resources, not generated output.
- For Xcode builds, disable **User Script Sandboxing** so the resource build phase can write generated files into the project directory.
- In debug Bootstrap maps `:res:<subdomain>/...` paths to the local `resources/` folder, which enables resource (for example, stylesheet) hot reload. Embedded resources are used when local files are unavailable, including release builds.
- If you rename the project/resource namespace, keep the resource subdomain aligned in the root element of `resources.xml`, the `K32("...")` argument to `Bootstrap::StartApp` in `code/entry.cpp`, and every `:res:<subdomain>/...` path.
