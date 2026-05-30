@ECHO off
SETLOCAL

SET SCRIPT_DIR=%~dp0
SET REFLEX_EXE=%SCRIPT_DIR%bin\tools\win\reflex.exe

IF NOT EXIST "%REFLEX_EXE%" (
    ECHO reflex.exe was not found at "%REFLEX_EXE%"
    ECHO Run install.bat first.
    EXIT /B 1
)

"%REFLEX_EXE%" %*
EXIT /B %ERRORLEVEL%
