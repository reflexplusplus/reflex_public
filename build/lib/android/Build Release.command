#!/bin/sh

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

try_java_home() {
	if test -d "$1"; then
		export JAVA_HOME="$1"
		return 1
	fi
	return 0
}

if ! test -d "$JAVA_HOME" &&
	try_java_home 'C:\Program Files\Android\Android Studio\jbr' &&
	try_java_home "/Applications/Android Studio.app/Contents/jbr/Contents/Home"
then
	echo "ERROR: Could not find the directory for the Java Virtual Machine. Please set the JAVA_HOME variable."
	if test "$REFLEX_NO_PAUSE" != "1"; then
		printf "\nPress Enter to close this window."
		read -r _
	fi
	exit 1
fi

cd "$SCRIPT_DIR" || exit 1

printf "\n*** Building ***\n"
sh gradlew clean assembleRelease
BUILD_EXIT=$?

if test "$BUILD_EXIT" -ne 0 && test "$REFLEX_NO_PAUSE" != "1"; then
	printf "\nBuild failed. Press Enter to close this window."
	read -r _
fi

exit "$BUILD_EXIT"
