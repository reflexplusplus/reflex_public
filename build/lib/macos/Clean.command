#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

printf "\n*** Cleaning ***\n"

xcodebuild -project "$SCRIPT_DIR/Reflex.xcodeproj" -scheme "Build Debug" -destination "generic/platform=macOS,name=My Mac" clean 1> /dev/null
BUILD_EXIT=$?

if [ "$BUILD_EXIT" -ne 0 ] && [ "$REFLEX_NO_PAUSE" != "1" ]; then
	printf "\nBuild failed. Press Enter to close this window."
	read -r _
fi

exit "$BUILD_EXIT"
