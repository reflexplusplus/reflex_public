@ECHO off

SET VSTUDIO="C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\devenv.exe"

SET SCRIPT_DIR=%~dp0

SET REFLEX_PATH=%SCRIPT_DIR%..\..\..


ECHO *** Building ***

%VSTUDIO% "%REFLEX_PATH%\build\lib\win\Reflex.sln" /clean "Debug|x64"

%VSTUDIO% "%REFLEX_PATH%\build\lib\win\Reflex.sln" /build "Debug|x64" /Out build.log

IF %ERRORLEVEL% NEQ 0 (
    ECHO *** Build failed ***
    PAUSE
    EXIT /B %ERRORLEVEL%
)

TYPE build.log
DEL build.log
