#!/bin/sh

SCRIPT_DIR=$(dirname -- "$( readlink -f -- "$0"; )")

printf "\n*** Building iOS device ***\n"
xcodebuild -project "$SCRIPT_DIR/Reflex-iOS.xcodeproj" -destination 'generic/platform=iOS' -scheme "Build Debug" clean build 1> /dev/null

printf "\n*** Building iOS simulator ***\n"
xcodebuild -project "$SCRIPT_DIR/Reflex-iOS.xcodeproj" -destination 'generic/platform=iOS Simulator' -scheme "Build Debug" clean build 1> /dev/null
