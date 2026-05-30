@echo off
REM Edit the path below if Android Studio is not installed to default location
set STUDIO_EXE="C:\Program Files\Android\Android Studio\bin\studio64.exe"

if exist %STUDIO_EXE% (
    start "" %STUDIO_EXE% "%CD%"
) else (
    echo Android Studio not found at %STUDIO_EXE%
    pause
)
