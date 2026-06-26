@echo off
setlocal

set SCRIPT_DIR=%~dp0
set "ANDROID_ROOT=%SCRIPT_DIR%"
for %%I in ("%ANDROID_ROOT%.") do set "ANDROID_ROOT=%%~fI"
set "REFLEX_ANDROID_PROJECT=%ANDROID_ROOT%\reflex"

if not defined JAVA_HOME (
    set "JAVA_HOME=C:\Program Files\Android\Android Studio\jbr"
)

cd /d "%SCRIPT_DIR%"

call :clean_project_state

echo *** Building ***
call gradlew.bat --no-build-cache --refresh-dependencies clean assembleDebug
set "BUILD_EXIT=%ERRORLEVEL%"

if not "%BUILD_EXIT%"=="0" if not "%REFLEX_NO_PAUSE%"=="1" pause
exit /b %BUILD_EXIT%

:clean_project_state
for %%D in (".gradle" ".cxx" ".externalNativeBuild" "reflex\build" "reflex\.cxx" "reflex\.externalNativeBuild") do (
    if exist "%ANDROID_ROOT%\%%~D" (
        echo Deleting %ANDROID_ROOT%\%%~D
        rd /s /q "%ANDROID_ROOT%\%%~D"
    )
)
exit /b 0
