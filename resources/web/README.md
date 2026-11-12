# Browser runtime

The browser port uses Clang/wasm-ld and explicit JavaScript imports. Compiled WASI libc/libc++/pthread/unwind support supplies the C++ ABI, malloc, thread stacks and TLS; browser filesystem/network services use the Reflex System bridge.

## Generated projects

Web targets use `launch_folder` to overlay custom launcher files onto the stock
`index.html` and `.htaccess`. Packaging gzip-compresses the public `.wasm` file
and substitutes `[BUILD_SIZE]` with its uncompressed byte count. The stock Apache
configuration and local server supply gzip and decoded-size headers.
Custom launcher URLs can use `./[FILENAME].wasm?[TIMESTAMP]` or a CDN prefix;
`[FILENAME]` is the URL-encoded output name without the extension.
See the [generated-project notes](../../build/tools/ReflexCLI/resources/project_generator/web_templates/README.md)
for all placeholders and custom hosting requirements.

The page URL's query parameters are passed to `OnStart` as command-line
arguments: `?preset=Demo&autoplay=false` becomes `--preset`, `Demo`, `--autoplay`,
`false`. Bare parameters such as `?debug` become flags (`--debug`); `?name=`
passes an explicit empty value. Names and values are URL-decoded (including
`+` as space), and repeated parameters retain their order. Empty names are
ignored. Only the page query is used, not the Wasm or runtime cache-key queries.

Use `reflex generate web` and `reflex build web` with Clang/LLVM helpers.
If changing the generator/templates, rebuild ReflexCLI and its embedded resources
before regenerating projects. See [generated-project notes](../../build/tools/ReflexCLI/resources/project_generator/web_templates/README.md).

System::Detail::Log and worker stdout/stderr are routed to the browser developer console.

Loading callbacks report decoded Wasm bytes. Compressed responses or responses without a usable length have an unknown total, so the standard entry pages show downloaded MiB instead of a percentage. A server may send `X-Uncompressed-Content-Length` containing the exact original Wasm byte length to enable percentages for compressed downloads (keep it synchronized with each build). Cross-origin hosting must expose that header through CORS. Completion reports 100% before compilation/application startup; cached downloads can complete too quickly to show intermediate updates.

`System::GetSystemID()` returns a nonzero random 64-bit browser installation ID stored in origin-local storage and supplied consistently to all workers. It is not a hardware ID. Clearing site data, switching profiles, or changing origin (including the localhost port) changes the identity. Use a fixed local server port when testing persisted activation. If storage is unavailable, the ID lasts for the process only.

## Build and run

Startup creates `/temp/`, `/user_data/` and `/user_documents/` in OPFS before
starting C++; existing contents are preserved. `/temp/` is currently persistent
OPFS storage too, with no automatic cleanup. `System::GetPath()` also returns
virtual paths for `/app_data/`, `/desktop/` and `/user_home/`, but startup does
not create those folders. Shipped application content normally uses embedded
resources rather than `/app_data/`.

`System::Open()` opens absolute HTTP/HTTPS URLs in a new browser tab (or window,
depending on browser settings). C++ rejects paths without an `http://` or
`https://` prefix before dispatching to JavaScript. The
page handles requests from both application and pthread workers and returns
`false` for unsupported URLs or popup blocking. Call it promptly in response to
a user click or key action; browser user-activation rules still apply. A `true`
result means the navigation was requested, not that the destination loaded.

From your project folder:

```powershell
reflex generate web
reflex build web debug
python projects/web/serve_local.py --configuration Debug
```

Open the URL printed by the local server. Generation emits Debug and Release configurations; choose the configuration with `reflex build web debug` or `release`.

Set `LLVM_ROOT` and `WASI_SYSROOT` for other installations. Windows defaults are LLVM 23.1.1 under `%LOCALAPPDATA%/Programs/LLVM-23.1.1/LLVM` and WASI SDK 34's sysroot under `%LOCALAPPDATA%/Programs/ReflexWasm/wasi-sysroot-34.0`. Matching compiler-builtins must be installed in Clang's target runtime directory. Standard builds use the generated CLI web projects.

Compile for `wasm32-wasip1-threads` with atomics/bulk-memory, standardized Wasm exceptions, Wasm setjmp support, `libsetjmp` and `libunwind`. All objects must use the same exception encoding; mixed legacy/new encodings fail browser validation. The application stack is 4 MiB because NanoJPEG has a large local decode table. Initial shared memory is 32 MiB, maximum 512 MiB; compiled malloc grows it as needed. Change the linker and page settings together when adjusting these prototype limits.

## Touch input

Set `touchMode: "raw"` (the default) in the launcher's `startReflex()` options to
forward every touch contact through the worker to `Window::Client::OnTouchBegin`,
`OnTouchMove`, `OnTouchEnd` and `OnTouchCancel`. Contacts retain their IDs, logical
coordinates, modifier keys and timestamps in the shared process timebase. Each
contact stays associated with its original logical window; destroying that
window or replacing its client cancels the contact. Browser cancellation,
lost capture, page hiding and focus loss also cancel active contacts. Touches
do not generate duplicate mouse input. Pen input retains mouse behaviour.

Set `touchMode: "viewport"` to let JavaScript handle two-finger pan and pinch
zoom of the canvas. Either finger can start on the canvas or on the background
of its parent element. Only single touches that start on the canvas go to Reflex.
When the second finger arrives, the forwarded contact is cancelled and the
gesture remains owned by JavaScript until all fingers lift. Existing native
actions already performed on touch-down cannot be undone by that cancellation;
GLX currently implements `OnTouchCancel` using its touch-end path.

Viewport mode owns the canvas CSS transform and transform origin. It scales the
displayed canvas between 0.25x and 8x, zooming around the gesture centre, without
resizing the C++ layout or its drawing buffer. Mouse and subsequent touch
coordinates are mapped back into logical canvas coordinates. `touch-action:
none` prevents the browser's own page gestures from competing with this mode.

`app.setTouchMode("raw")` and `app.setTouchMode("viewport")` switch modes at
runtime. Switching during a touch sequence cancels native contacts and waits
until all fingers lift before applying the new mode. The visual transform is
preserved across mode changes; call `app.resetTouchViewport()` to restore the
original canvas transform and zoom. The existing mouse/keyboard/wheel bridge
continues to operate independently.

## Execution and ownership

System uses separate `src/reflex/system/web_app.cpp` and `web_audioapp.cpp`
umbrellas, sharing `web/build.cpp`. `ReflexTargetApp` and
`ReflexTargetAudioApp` select their respective sources; the System backend does
not use Bootstrap extension defines to choose its application implementation.

`runtime.js` compiles the module, creates shared memory, transfers the canvas and starts application/I/O workers. Serve COOP `same-origin` and COEP `require-corp`; the local server supplies both. Only the application worker initializes the reactor/C++ constructors and Reflex modules. Export `__wasm_call_ctors` to suppress automatic constructor wrappers. Secondary workers enter `wasi_thread_start`; linker-generated passive-data initialization coordinates shared process memory.

The loader supplies one process time origin to the application worker and every managed thread. `System::GetElapsedTime()` and the WASI monotonic clock normalize each worker's local performance clock to that origin, so timestamps can be compared across threads.

`application-worker.js` owns WebGL2 and invokes the System clock through worker animation frames. `gl.js` supplies the ES3 ABI used by the common renderer with integer object handles and refreshed memory views. `input.js` decides DOM defaults on the page, then forwards events to System. Renderer/window operations require the application worker; secondary renderer imports fail explicitly when invoked.

`threads.js` maps each dynamic pthread creation to a new browser worker, terminated when the thread finishes. The calling worker waits for readiness while the page remains available. Priorities, pooling and abnormal worker recovery remain future work.

`io-worker.js` owns OPFS handles and fetch streams. `io.js` uses per-request shared completion records, routed by the page without blocking. File copies are bounded to 64 KiB. HTTP bodies are pulled into bounded caller buffers with backpressure; callbacks execute synchronously on the calling Reflex thread. CORS, restricted headers, quotas and OPFS locks remain browser constraints. Read-only handles can coexist; writable handles use browser locking even for `lock=false`. Timestamp setters are unsupported; rename requires browser `move` support.

`libc.js` implements the small support ABI needed by compiled C/C++: empty environment/arguments, clocks, random bytes, console streams, yield, worker-safe clock polling and exit reporting. It does not emulate a general WASI filesystem. Math uses the compiled standard library rather than JavaScript imports. Missing imports fail instantiation; the linker allows only declared host GL imports, so unresolved Reflex symbols fail the build.

## Current limits

The Project UI renders and accepts mouse, wheel, touch and basic keyboard input. Multiple logical windows share one canvas and WebGL context; a runtime-owned header selector switches the visible/input window, preserving each client's state and requested dimensions. Only the selected window renders; no cross-context resource sharing is needed. System mouse cursors map to CSS cursor keywords on the page canvas, with each window's cursor restored on selection. Stereo output advertises one synthetic `Web Audio` device; Fit, density updates, drag pointer capture and virtual cursor recentering are implemented. Audio input, IME, clipboard, external pickers, real fullscreen, capture-loss handling and drag/drop remain unfinished. Simultaneous multi-window display and non-RGBA8 images are unsupported; WebGL context-loss recovery is not implemented.


The application lives for the page's lifetime; there is no public stop/restart API or graceful native shutdown requirement. The browser owns teardown when the document is discarded. Internal startup-failure cleanup still terminates acquired workers. Abnormal pthread-worker exits can strand joins, and post-start application errors do not perform full process cleanup; these remain outstanding failure-handling fixes. Canvas transfer is one-shot; replace the DOM canvas before retrying startup. Persist during ordinary operation instead of relying on page unload or C++ destructors.

### Stereo output

The web audio backend's `OnResume()` requests AudioContext creation/resume through
the page bridge. Initialization is asynchronous and shared by repeated resume
requests, including the lock used to configure the actual browser sample rate.
There is no launcher Start overlay. If autoplay is blocked, a click or key action
on the app canvas retries the pending resume directly in that DOM event handler.
`OnPause()` still synchronously stops DSP production; it does not suspend the
browser AudioContext or discard already queued output. Legacy `runAudio()` and
`pauseAudio()` JavaScript helpers remain available for custom launchers.
Reflex DSP runs in a managed pthread worker at the context's actual sample rate,
producing 512-frame stereo blocks.
A nonblocking AudioWorklet consumes a shared ring; shortages output silence.
The selectable buffer sizes are 512, 1024, 1536 and 2048 queued frames. The default
is 2048 frames (approximately 43 ms at 48 kHz); DSP blocks remain 512 frames.
The producer waits for consumption and pause/resume notifications, retaining a
four-block ring capacity. Reducing the target lets existing queued audio drain.
Browser/device latency is additional; measure underruns under realistic load
before settling on a production default. Audio input is not implemented. The worklet refreshes typed views after memory growth.


### MIDI

The Bootstrap MIDI panel requests access when it attaches to a window. Custom
panels can request access asynchronously with
`Data::SetBool(audioapp, "request_midi", true)`. On success `GetMidiPorts` lists
the browser's ports and `System::kNotificationChangeDevices` signals the update.
The browser must support Web MIDI and permit access in a secure context (HTTPS or
localhost); embedded launchers also need the `midi` Permissions Policy permission.
Unavailable or denied access leaves the port lists empty and audio continues.
Access/open/send failures are reported in the browser console.

The existing `GetMidiPorts` and `AudioPlugin::Lock::EnableMidiPort` APIs enumerate
and select browser MIDI inputs and outputs. Ports start disabled. Selection follows
browser port IDs across enumeration changes; disconnection disables the port and
reconnection requires enabling it again. Input events enter a bounded 256-event
queue and are delivered at position zero in the next DSP block. Already queued
audio still contributes the configured output latency. Output MIDI is sent
immediately to every enabled output, matching the standalone desktop routing;
output event sample offsets are not scheduled against browser audio playback.

Only short MIDI 1.0 messages (one to three bytes) are supported by the Reflex event
payload. Message lengths are preserved on output, including program changes,
channel pressure, and system realtime messages. SysEx access is not requested.
