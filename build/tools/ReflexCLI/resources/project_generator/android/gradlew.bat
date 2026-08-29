@echo off
setlocal
set "APP_HOME=%~dp0"
if not defined ANDROID_HOME (
	if not defined ANDROID_SDK_ROOT (
		if exist "%LOCALAPPDATA%\Android\Sdk" (
			set "ANDROID_HOME=%LOCALAPPDATA%\Android\Sdk"
			set "ANDROID_SDK_ROOT=%LOCALAPPDATA%\Android\Sdk"
		)
	)
)
if defined JAVA_HOME (
	if exist "%JAVA_HOME%\bin\java.exe" set "JAVA_EXE=%JAVA_HOME%\bin\java.exe"
)
if not defined JAVA_EXE (
	if exist "C:\Program Files\Android\Android Studio\jbr\bin\java.exe" set "JAVA_EXE=C:\Program Files\Android\Android Studio\jbr\bin\java.exe"
)
if not defined JAVA_EXE (
	where java.exe >nul 2>nul
	if not errorlevel 1 set "JAVA_EXE=java.exe"
)
if not defined JAVA_EXE (
	echo Java was not found. Set JAVA_HOME or install Android Studio.
	exit /b 1
)
pushd "%APP_HOME%" || exit /b 1
"%JAVA_EXE%" -classpath "%APP_HOME%gradle\wrapper\gradle-wrapper.jar" org.gradle.wrapper.GradleWrapperMain %*
set "GRADLE_EXIT_CODE=%ERRORLEVEL%"
popd
endlocal & exit /b %GRADLE_EXIT_CODE%
