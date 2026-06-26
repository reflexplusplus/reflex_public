# Building
* You can open the directory with Visual Studio Code or CLion, or no IDE at all.


## CLion
* Open the directory (`webasm`) with the IDE.
** In the dialog that will display (or later under **Settings** -> **Build, Execution, Deployment** -> **CMake**), use the default toolchain, and all default options, just put
`-DCMAKE_TOOLCHAIN_FILE=${EMSDK}/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake` to the **CMake Options** field, and set the **build directory** to `build/Debug` (because it's a debug configuration, adjust if needed).
* After that, you can select `AllLibraries` as a configuration and click on the hammer to build it (there is no executable to run).

Nota bene: if you have any issue, delete the `build` directory inside `webasm`, then right-click on the project name (`webasm`) in the Project explorer, and select **Reload CMake Project**.


## VS Code
* Just open the project. Cancel any prompt to find an SDK. Make sure that you have the C/C++ Extension installed and enabled (the official one from Microsoft).
* You should be able to open the palette (Ctrl+Shift+P / Cmd+Shift+P) and type `Run Task`, then select `Build project (Debug, emscripten)`.


## No IDE
* Just run `build.sh` and `clean.sh` as needed.
* 💡 On Windows, you can run them from a Git Bash shell, or directly (from a native Command Prompt) by either adding the git bash shell to the PATH, or by specifying the full path, for example run: `"C:\Program Files\Git\usr\bin\sh.exe" build.sh`, having `cd` in the `webasm` directory first.


