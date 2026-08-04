@ECHO off

SET VSTUDIO="C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\devenv.exe"
SET SCRIPT_DIR=%~dp0
SET SOLUTION=%SCRIPT_DIR%ReflexCLI.sln

ECHO *** Cleaning ***

%VSTUDIO% "%SOLUTION%" /clean "Release|x64"

ECHO *** Building ***

%VSTUDIO% "%SOLUTION%" /build "Release|x64" /Out build.log

IF %ERRORLEVEL% NEQ 0 (
    ECHO *** Build failed ***
    PAUSE
    EXIT /B %ERRORLEVEL%
)

TYPE build.log
DEL build.log

EXIT /B 0
