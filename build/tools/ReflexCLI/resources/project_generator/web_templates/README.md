# Generated web projects

`reflex generate web --path project.cfg` emits an LLVM build under the
project's `generated_directory/web/` (normally `projects/web/`). CMake remains
an independent generation option.

Generated files:

- `build-config.json`: resolved sources, compile options, libraries, output
  paths and pre/post-build actions from `project.cfg`.
- `build.py`: `clang++` compilation/linking and `llvm-ar` archives.
- `build_<configuration>.bat` and `.command`: helpers used by `reflex build`.
- `run_local_debug.bat` / `.command` and `run_local_release.bat` / `.command`:
  build, serve locally and open the default browser for application projects.
- `launchers/<configuration>/<target>/index.html`: rendered launcher copied into
  the selected application's output folder on each build.
- `.htaccess`: written beside the output `index.html` on each application build,
  setting the directory index, COOP/COEP isolation headers, Wasm/JavaScript MIME
  types and cache headers on Apache hosts. `index.html` revalidates on each
  use. All other files, including JavaScript and Wasm, are cacheable for one day.
- `serve_local.py`: serves a selected built app with COOP/COEP and Wasm MIME headers.

With a ReflexCLI containing the web generator (rebuild CLI/resources after
editing the generator or these templates):

```powershell
reflex generate web --path path/to/project.cfg
reflex build web Release --path path/to/project.cfg
python path/to/projects/web/serve_local.py --configuration Release
```

Use `--target <target-name>` with `serve_local.py` when multiple applications are
present. The run helpers forward arguments, for example:

```powershell
.\projects\web\run_local_debug.bat --target Standalone --port 8081
```

On macOS/Linux, use `projects/web/run_local_debug.command` (or the release
helper). Helpers are emitted for available Debug/Release application
configurations; static-library-only projects do not receive run helpers. App
selection is checked before building. Build failures stop startup, and the
browser opens only after the server has successfully bound its port. By default,
the server binds port 0, letting the OS choose an available localhost port; the
printed URL and browser use that actual assigned port. Pass `--port N` to choose
a specific port. Windows uses exclusive socket binding, so an occupied explicit
port fails rather than overlapping another server. Press
Ctrl+C in the helper's terminal to stop serving. `serve_local.py` also accepts
`--build` and `--open` directly; without these flags it just serves existing
output.

Invoke `build.py` directly for `--jobs <n>` or `--rebuild`.

Select a launch folder in the target's `@web` configuration with
`launch_folder: "platform_assets/web/";`. This replaces the old `entry_page`
setting. It follows normal generator path resolution rules; a configured folder
must exist. Without the setting, generation uses the stock launcher.
Generation installs the stock `index.html` and `.htaccess`, then recursively
overlays the launch folder. Custom `.htaccess` files replace the stock file.
The standard app configurations select `platform_assets/web/`; Widgets selects
`project_assets/web/`. Images, scripts and other assets are included too.

Generation substitutes these placeholders in `.htaccess`, HTML, CSS, JavaScript,
JSON, SVG and text files, including files supplied by the user:

- `[TITLE]`: HTML-escaped project name.
- `[FILENAME]`: percent-encoded output filename without `.wasm`.
- `[MODULE]`: percent-encoded relative Wasm URL (`./[FILENAME].wasm`).
- `[MODULE_FILENAME]`: complete Wasm filename, escaped for an Apache quoted value.

The default module URL is `./[FILENAME].wasm?[TIMESTAMP]`. A custom launcher can
instead use `https://some.cdn/[FILENAME].wasm?[TIMESTAMP]`; its CDN must supply
the required response headers and allow the page to fetch the resource.
During packaging, `build.py` substitutes `[TIMESTAMP]` and `[BUILD_TIMESTAMP]`
with the invocation's Unix time in milliseconds, and `[BUILD_SIZE]` with the
uncompressed Wasm byte count. All apps in an invocation share the timestamp,
which changes even when no relinking is needed. Binary assets are copied unchanged.

Packaging gzip-compresses the linked Wasm by default, retaining the `.wasm`
filename. The uncompressed linker output stays under `intermediate/` so
incremental builds work normally. The stock `.htaccess` supplies
`Content-Type: application/wasm`, `Content-Encoding: gzip` and
`X-Uncompressed-Content-Length: [BUILD_SIZE]` for `[MODULE_FILENAME]`.
Preserve these headers in a custom `.htaccess` or equivalent CDN configuration.
The local Python server also supplies them.

Source files are never overwritten. Source edits require regeneration;
for temporary experiments, edit the generated folder under `launchers/`.
Packaging substitutes build values into output files without changing that folder.
Internal runtime imports, Worker URLs and audio worklet URLs use a shared
revision query (currently `?r6-dev`). Increment it across all internal URLs when
deploying runtime JavaScript changes. For local edits under an unchanged
revision, disable or clear the browser cache. Packaging copies the runtime
modules unchanged except for `thread-worker.js`, which it combines with
`worker-imports.js` into a self-contained script. New pthread workers load
that script directly from its versioned URL.
The local server serves static files and does not execute PHP.

Set `LLVM_ROOT` to an LLVM installation containing `clang++`, `wasm-ld` and
`llvm-ar`, and `WASI_SYSROOT` to the threaded WASI SDK sysroot. Matching
compiler-builtins must be installed in Clang's target runtime directory. The
Windows defaults match the WebAssembly prototype: LLVM 23.1.1 and WASI SDK 34's
sysroot under `%LOCALAPPDATA%/Programs/`. Other hosts require a configured
sysroot; LLVM tools can also be discovered on PATH.

Escaping is specific to each output format: the emitter uses `JsonString` /
`JsonStringArray` for the manifest and `BatchArgument` for Windows helper
arguments. Batch helpers disable delayed expansion and escape literal percent
characters. Configuration names containing quotes or control characters are
rejected because they also name generated files. POSIX helpers use the shared
shell quoting helper. The Python builder applies `llvm_response_argument` only
when writing LLVM response files; arguments passed to subprocess remain raw.

Compilation runs in parallel with Clang-generated header dependency tracking.
Dependency-file parsing decodes specific Make escapes, preserves native Windows
separators and normalizes decoded Windows paths to forward strokes. Malformed
or empty dependency files cause a conservative rebuild.
Changing compile options invalidates affected objects. Linking tracks objects,
library timestamps and command changes. Response files accommodate long command
lines. Paths keep Reflex's canonical forward strokes; Python-created paths use
`Path.as_posix()`, including environment-supplied LLVM/WASI paths. Arguments
still use LLVM/GNU escaping to preserve spaces, quotes and literal backslashes
in compiler definitions. Response-format changes invalidate cached
objects and link commands. Pre/post-build actions execute on every build, so referenced resource
assets are not missed. Static archives are recreated when relinking to remove
objects belonging to deleted source files.

Applications use the configured output directory and produce `<OutputName>.wasm`,
`index.html` and `runtime/*.js`. Static targets produce `lib<OutputName>.a` and do
not package browser files. Source dependencies use the normal project graph;
SDK libraries live under `bin/lib/web/<Configuration>/`. Release builds publish
these Debug/Release archives as `reflex-libs-web.zip` for both `reflex` and
`reflex_public`, and include them in the desktop SDK distributions. The public
archive excludes the VM libraries, following the other platforms. These are
wasm32 static libraries, shared across Windows, macOS and Linux hosts; consumers
still need a compatible LLVM/WASI toolchain to compile their own code and link
the final `.wasm` application. The CI jobs use WASI SDK 34. This emitter does not
install LLVM/WASI automatically.

Upload the app output folder to an HTTPS host with:

```
Cross-Origin-Opener-Policy: same-origin
Cross-Origin-Embedder-Policy: require-corp
```

Include the generated `.htaccess` when uploading to Apache. Its cache rules
require `mod_headers` and `AllowOverride FileInfo` (or an equivalent
`AllowOverrideList` permitting `Header`). Configure equivalent rules for other
hosts. If a CDN caches these files, include the full query string in its cache
key so the launcher timestamps and JavaScript revisions select fresh assets.
The local Python server sends `no-store` for development and ignores `.htaccess`.

Serve `.wasm` as `application/wasm`. The launcher uses relative URLs. Set
`initialBytes` and `maximumBytes` in the launcher's `startReflex` call to
control the startup size and growth limit of shared memory. The supplied
launchers use 32 MiB and 512 MiB. The linker computes the minimum from the
app's static memory layout, including its 4 MiB stack. Startup fails if
`initialBytes` is below that minimum. The linked Wasm allows an imported
maximum up to the 4 GiB wasm32 ceiling, subject to browser limits. Both
launcher values must be multiples of 64 KiB, and the maximum must be at least
the initial size.

The emitter currently supports `app` and `static_library`, with `architectures:
native` selecting wasm32. Native console and plugin targets are disabled for
web in the standard configurations. Ordinary UI and audio applications use
separate backend archives. Audio controls are shown only when the module exports
audio support. The existing JS runtime is copied without bundling. Engine-only
startup and splitting System common/UI remain separate platform work.

Edit `project.cfg` and regenerate to change sources/settings; generated files
are disposable. The CLI resource XML embeds these templates; regenerate CLI
resources through the normal resource build rather than editing generated C++.
# Release stripping

Web app links for the `Release` configuration use `--strip-all` to remove optional symbol/debug metadata. Debug retains metadata. Required Wasm imports and exports remain available, but Release browser stack traces generally show function indices instead of C++ names. HTTP compression can further reduce transfer size.
