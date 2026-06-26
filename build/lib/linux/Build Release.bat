@echo off
setlocal

set "WIN_BUILD_DIR=%~dp0"
if "%WIN_BUILD_DIR:~-1%"=="\" set "WIN_BUILD_DIR=%WIN_BUILD_DIR:~0,-1%"

for /f "usebackq delims=" %%I in (`wsl.exe wslpath -a "%WIN_BUILD_DIR%"`) do set "WSL_BUILD_DIR=%%I"
if not defined WSL_BUILD_DIR (
    echo Failed to convert "%WIN_BUILD_DIR%" to a WSL path.
    set "BUILD_EXIT=1"
    goto finish
)

wsl.exe -- bash -lc "cd '%WSL_BUILD_DIR%' && make clean release"
set "BUILD_EXIT=%ERRORLEVEL%"

:finish
if not "%BUILD_EXIT%"=="0" if not "%REFLEX_NO_PAUSE%"=="1" pause
exit /b %BUILD_EXIT%
