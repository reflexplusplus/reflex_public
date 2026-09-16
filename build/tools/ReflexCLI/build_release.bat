@echo off
setlocal

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
for /f "usebackq tokens=*" %%I in (`"%VSWHERE%" -latest -products * -property installationPath`) do set "VS=%%I"
if not defined VS exit /b 1

set "ARCH=%PROCESSOR_ARCHITECTURE%"
if defined PROCESSOR_ARCHITEW6432 set "ARCH=%PROCESSOR_ARCHITEW6432%"

if /i "%ARCH%"=="ARM64" (
    call "%VS%\VC\Auxiliary\Build\vcvarsall.bat" arm64
) else (
    call "%VS%\VC\Auxiliary\Build\vcvarsall.bat" amd64
)
if errorlevel 1 exit /b %errorlevel%

for %%I in ("%~dp0.") do set "SOURCE_DIR=%%~fI"
set "BUILD_DIR=%SOURCE_DIR%\output\cmake-release"

cmake -S "%SOURCE_DIR%" -B "%BUILD_DIR%"
if errorlevel 1 exit /b %errorlevel%

cmake --build "%BUILD_DIR%" --config Release --parallel
if errorlevel 1 exit /b %errorlevel%
