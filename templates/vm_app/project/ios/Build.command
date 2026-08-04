#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

printf "\n*** Cleaning ***\n"

xcodebuild -project "$SCRIPT_DIR/_PRODUCT-NAME_ (iOS).xcodeproj" -scheme "App" -configuration Release -destination 'generic/platform=iOS' clean 1> /dev/null
xcodebuild -project "$SCRIPT_DIR/_PRODUCT-NAME_ (iOS).xcodeproj" -scheme "App" -configuration Release -destination 'generic/platform=iOS Simulator' clean 1> /dev/null

printf "\n*** Building iOS device ***\n"

xcodebuild -project "$SCRIPT_DIR/_PRODUCT-NAME_ (iOS).xcodeproj" -scheme "App" -configuration Release -destination 'generic/platform=iOS' build 1> /dev/null
BUILD_EXIT=$?
if [ $BUILD_EXIT -ne 0 ]; then
    printf "*** Build failed ***\n"
    exit $BUILD_EXIT
fi

printf "\n*** Building iOS simulator ***\n"

xcodebuild -project "$SCRIPT_DIR/_PRODUCT-NAME_ (iOS).xcodeproj" -scheme "App" -configuration Release -destination 'generic/platform=iOS Simulator' build 1> /dev/null
BUILD_EXIT=$?
if [ $BUILD_EXIT -ne 0 ]; then
    printf "*** Build failed ***\n"
    exit $BUILD_EXIT
fi

exit 0
