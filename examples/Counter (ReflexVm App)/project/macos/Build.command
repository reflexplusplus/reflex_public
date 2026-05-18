#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

printf "\n*** Cleaning ***\n"

xcodebuild -project "$SCRIPT_DIR/Counter (ReflexVm App).xcodeproj" -scheme "Build" -configuration Release -destination "generic/platform=macOS,name=Any Mac" clean 1> /dev/null

printf "\n*** Building ***\n"

xcodebuild -project "$SCRIPT_DIR/Counter (ReflexVm App).xcodeproj" -scheme "Build" -configuration Release -destination "generic/platform=macOS,name=Any Mac" build 1> /dev/null
BUILD_EXIT=$?

if [ $BUILD_EXIT -ne 0 ]; then
    printf "*** Build failed ***\n"
    exit $BUILD_EXIT
fi

exit 0
