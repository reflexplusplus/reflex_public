# Issues

## VS Code generation omits Linux pkg-config include paths

Observed in `examples/Widgets` on Linux after running:

```sh
../../bin/tools/linux/reflex generate linux --path project.cfg
```

The generated `.vscode/c_cpp_properties.json` and both `.vscode/compile_commands_*.json` omit D-Bus include paths, causing IntelliSense to report:

```text
cannot open source file "dbus/dbus.h"
```

The D-Bus development headers are installed. `pkg-config --cflags dbus-1` returns:

```text
-I/usr/include/dbus-1.0 -I/usr/lib/x86_64-linux-gnu/dbus-1.0/include
```

A standalone compiler check including `<dbus/dbus.h>` with these flags succeeds. Regenerating with the installed CLI still leaves both paths out of the VS Code settings and compile databases.

Expected behavior: resolve Linux `pkg-config --cflags` for each target and its link dependencies, and propagate the flags to the generated IntelliSense settings and compilation databases for Debug and Release.

Investigation: `reflex_source` was disabled on the affected Linux machine. That
selects the imported SDK library declarations in `project_configs/common_app.cfg`,
even when framework source and successfully built libraries exist locally. Those
declarations lacked the `pkg_config` metadata supplied by `build/lib/project.cfg`.
The VS Code emitter already queries target and link-dependency packages, but had
no packages to query in SDK mode.

Fix: Linux package metadata now lives in shared templates in
`project_configs/common.cfg`, inherited by both the source and imported SDK
library declarations, including UI packages (D-Bus) and audio packages (ALSA).
This also supplies the metadata to the Linux build emitter. Linux regeneration
and IntelliSense verification are still pending; no build or runtime tests have
been run for this fix. The existing combined pkg-config query still requires
all declared development packages to be installed.

Local workaround: the Widgets VS Code include paths and both compilation databases were manually updated with the two D-Bus include directories. Regeneration with the affected CLI overwrites this workaround.
