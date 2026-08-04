@echo off
echo Cleaning Android project...

set DIRS_TO_DELETE=build .gradle .cxx intermediates obj libs prefab

for %%F in (%DIRS_TO_DELETE%) do (
    for /d /r %%d in (%%F) do (
        if exist "%%d" (
            echo Deleting %%d
            rd /s /q "%%d"
        )
    )
)

set LOCAL_GRADLE_CACHE=%USERPROFILE%\.gradle\caches
if exist "%LOCAL_GRADLE_CACHE%" (
    echo Deleting local Gradle cache (can be large): %LOCAL_GRADLE_CACHE%
    rd /s /q "%LOCAL_GRADLE_CACHE%"
)
