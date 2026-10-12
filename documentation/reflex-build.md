# Reflex Generate and Build System

Reflex `generate` command turns a declarative `project.cfg` into native build projects for
Visual Studio, Xcode, Android, Linux, web, or CMake. The cfg file is the source
of truth; generated projects are disposable output.

## Workflow

1. **Bootstrap the `reflex` tool.**
   - **`reflex_public` (without framework source):** Use the bootstrap
     download scripts provided on the [Reflex++ download page](https://reflexplusplus.dev/download).
   - **`reflex` (full source repository):** Build the tool using the provided
     scripts in `build/tools/ReflexCLI/`: `build_release.bat` on Windows or
     `build_release.command` on macOS. These require CMake and the platform's
     compiler toolchain.
   - Make the resulting tool available on `PATH`.
2. **Create → generate → build.**
   - **`reflex create`:** Installs a project template and generates its
     platform projects.
   - **`reflex generate`:** Updates those projects from the cfg description.
   - **`reflex build`:** Invokes the platform's build tools.
   - **`reflex edit`:** Opens a generated IDE project instead.

For example, from a terminal with the Windows toolchain available:

```text
mkdir MyApp
cd MyApp
reflex create
```

Follow the prompts to choose a template, enter the vendor and product names,
and select the current folder as the output location. Then build Debug:

```text
reflex build windows debug
```

Or open the generated project and build from the IDE:

```text
reflex edit
```

`create` runs generation automatically the first time. After editing
`project.cfg` or an included cfg file, regenerate before building:

```text
reflex generate
```

By default, generation selects all enabled project platforms supported by the
current machine's host defaults: Windows selects Windows, Android, and
web; macOS selects macOS, iOS, Android, and web; Linux selects Linux, Android,
and web. Generating a project does not require building it, and each platform
still needs its own toolchain to compile. To select a particular output, use
`reflex generate windows` or `reflex generate cmake`.

`reflex build` defaults to the host's native platform (Windows, macOS, or Linux)
and builds all configurations. Use `reflex build windows debug` to select one
platform and configuration, or `reflex build debug` to build Debug for the native
platform. `reflex build all` uses the broader default platform selection above;
`reflex build all debug` limits those builds to Debug. Generate the selected
projects first; each build requires its platform toolchain.

If a platform build fails, the CLI reports its error and continues with the
remaining selected platforms. The command exits with a failure status if any
platform failed, including when a required toolchain is missing. Linux remains
available explicitly on Windows with `reflex generate linux` and
`reflex build linux`, using a configured WSL toolchain. See the
[Linux and WSL toolchain setup](toolchain.md#reflex-build-linux) for prerequisites.

`reflex generate cmake` writes a root `CMakeLists.txt`. Configure and build it
with CMake directly; `reflex build` dispatches native platform builds:

```text
cmake -S . -B build/cmake-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build/cmake-debug --config Debug
```

The Linux Makefile generator supports apps, console applications, static libraries,
and dynamic libraries, including CLAP, VST2, and VST3 plugins. It links products
against their Reflex archives and the archives' `pkg_config` dependencies.
Linux configurations accept one `native` or `x64` architecture. Dynamic libraries
use `-fPIC` and `-shared`; `extension` and `output_directory` select the product
filename and bundle layout. The stock Linux VST3 template targets x64 and places
its `.so` in `$(OutputName).vst3/Contents/x86_64-linux/`. Declare Linux system
dependencies with `pkg_config` inside `@linux` to
add their compilation and linking flags.

## How the system fits together

The generator is designed as a general project-generation system, independent
of the Reflex framework. Projects describe targets, files, dependencies, and
build settings; Reflex applications obtain their framework-specific defaults
by including the stock cfg files.

The description is declarative rather than a script. Includes assemble shared
definitions into a project, templates provide reusable target settings, and
target → platform → configuration scopes specialize those settings. This lets
a common definition serve several products, operating systems, and build
configurations without duplicating native project files.

Includes form an ordered graph. Each document keeps its own location, so
relative includes and target paths remain anchored to the document that owns
them. Include paths are expanded in order: variables from an earlier include
are available when locating the next one.

PropertySheet variables (`@Variables`) provide defaults and derived values.
References such as `$(REFLEX_PATH)` are resolved in the applicable scope;
included defaults can be overridden by the including document and more
specific target settings. Persistent CLI variables, managed with
`reflex set`, `get`, and `unset`, override cfg variables and are also
available to `#if` conditional branches. Together, includes, inheritance,
variables, and conditional selection let the same description cover source
and prebuilt dependencies, optional features, and different local setups.

Two built-in expressions help stock projects locate Reflex:
`$[get_reflex_path:]` discovers the SDK from the CLI's location, and
`$[get_reflex_cli_path:]` returns the running executable's path. Stock configs
use these as defaults for `REFLEX_PATH` and `REFLEX_CLI_PATH`; persistent
settings can override them. These hooks are conveniences for Reflex projects,
rather than requirements of the general cfg system.

Generation resolves the description and dependency graph, emits the requested
platform projects, and runs any post-generation actions. `reflex build` then
delegates compilation to the platform's tools and builds available source-project
dependencies in order. Keep changes in the cfg files and source assets, then
regenerate, rather than editing generated projects.

## Reflex default configs and standard variables

The stock files in `project_configs/` are a layer of Reflex conventions built
on top of the general generator. They centralize framework dependencies,
platform defaults, and application build steps, keeping each end project's
`project.cfg` small.

| File | Role |
| --- | --- |
| `common.cfg` | Shared variables, compiler settings, and platform/configuration defaults used by both the Reflex library build project and Reflex-based apps and libraries |
| `common_app.cfg` | Reflex source or prebuilt library dependencies, shared application settings, and resource generation |
| `cpp_app.cfg` | Native GUI application template and platform assets |
| `cpp_console_app.cfg` | Console application templates |
| `cpp_audioplugin.cfg` | Audio plugin and standalone application templates |
| `vm_app.cfg` | ReflexVm application settings layered over the native application template |

`reflex create` copies a project template from `templates/`. Its resulting
`project.cfg` includes the appropriate stock config and declares concrete
targets that inherit its templates. The end project supplies product metadata,
source files, assets, and any local overrides; the included configs supply the
shared build definitions. These conventions are optional. For full control,
maintain your own cfg definitions and include graph, since repository defaults
may change as Reflex evolves.

`REFLEX_PATH` identifies the SDK used by config includes, sources, headers, and
libraries. Stock project templates define an automatic default using
`$[get_reflex_path:]`, then locate their stock config through
`$(REFLEX_PATH)`. A persistent override applies throughout config resolution:

```text
reflex set REFLEX_PATH "/custom/path/reflex"
```

`REFLEX_CLI_PATH` defaults to the running CLI executable and supplies commands
such as resource and plist generation. These config variables do not change
the executable-based SDK discovery used by `reflex where` or template lookup.

The standard `reflex_source` CLI variable controls `#if` branches in the stock
configs. Set `reflex_source` to `true` to include the SDK's library source
project and build Reflex targets from source; set it to `false` to select
prebuilt library declarations. Those declarations can still build an available
`source_project` dependency when needed. After changing either selection,
run `reflex generate` again.

Windows applications and framework libraries default to x64 only. The optional
`reflex_32bit` variable enables x86 alongside x64 in the shared defaults:

```text
reflex set reflex_32bit true
reflex generate windows
```

Set it to `false` or run `reflex unset reflex_32bit` to return to x64-only
generation. Regenerate Windows projects after changing the setting. Individual
projects can still override their `architectures` explicitly.

## Property reference

The detailed cfg types, properties, and supported values are documented in the
`reflex_generate` codebase. Select it and browse or search:

```text
reflex doc codebase reflex_generate
reflex doc list --root reflex_generate
reflex doc list optimization --root reflex_generate
```

Use `reflex doc info <symbol> --root reflex_generate` to inspect an entry.
`--root` is optional and mainly intended for agents: it makes each query
independent of the saved codebase and browsing-location preferences. For
interactive use, omit it to browse within the current saved location.
