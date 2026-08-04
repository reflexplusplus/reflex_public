@echo off
rem Thin launcher for install.ps1 (the real Windows installer). Fetches the
rem prebuilt Reflex SDK for Windows from the repo's GitHub Release.
setlocal
set "ROOT=%~dp0"
powershell -NoProfile -ExecutionPolicy Bypass -File "%ROOT%install.ps1"
exit /b %errorlevel%
