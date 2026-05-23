@echo off
setlocal EnableDelayedExpansion

set ROOT=%~dp0
set VERSION_FILE=%ROOT%version.txt
set INSTALLED_VERSION_FILE=%ROOT%bin\version.txt

if not exist "%VERSION_FILE%" (
	echo Missing version.txt
	exit /b 1
)

set /p VERSION=<"%VERSION_FILE%"

if exist "%INSTALLED_VERSION_FILE%" (
	set /p INSTALLED_VERSION=<"%INSTALLED_VERSION_FILE%"

	if "!INSTALLED_VERSION!"=="%VERSION%" (
		echo Reflex SDK %VERSION% binaries already installed.
		exit /b 0
	)
)

set URL=https://reflexplusplus.b-cdn.net/download/sdk?platform=win^&version=%VERSION%
set ZIP=%TEMP%\reflex-sdk-%VERSION%-win.zip
set TEMP_DIR=%TEMP%\reflex-sdk-%VERSION%-win

echo Downloading Reflex SDK %VERSION% binaries...

powershell -NoProfile -ExecutionPolicy Bypass -Command ^
	"Invoke-WebRequest -Uri '%URL%' -OutFile '%ZIP%'"

if errorlevel 1 exit /b %errorlevel%

if exist "%TEMP_DIR%" rmdir /S /Q "%TEMP_DIR%"
mkdir "%TEMP_DIR%"

echo Extracting...

powershell -NoProfile -ExecutionPolicy Bypass -Command ^
	"Expand-Archive -Path '%ZIP%' -DestinationPath '%TEMP_DIR%' -Force"

if errorlevel 1 exit /b %errorlevel%

if exist "%TEMP_DIR%\bin" (
	xcopy "%TEMP_DIR%\bin" "%ROOT%bin\" /E /I /Y >NUL
) else (
	echo Package does not contain bin folder.
	exit /b 1
)

copy /Y "%VERSION_FILE%" "%ROOT%bin\version.txt" >NUL

del "%ZIP%"
rmdir /S /Q "%TEMP_DIR%"

echo Reflex SDK %VERSION% binaries installed.

endlocal
