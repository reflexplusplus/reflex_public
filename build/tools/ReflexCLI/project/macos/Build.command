#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUTPUT_DIR="$SCRIPT_DIR/output/release"
DERIVED_DATA_DIR="$SCRIPT_DIR/output/deriveddata"
LOG_DIR="$SCRIPT_DIR/output/logs"

mkdir -p "$LOG_DIR"

run_xcodebuild() {
	local label="$1"
	shift
	local log_path="$LOG_DIR/${label}.log"

	if ! xcodebuild "$@" >"$log_path" 2>&1; then
		echo
		echo "*** ${label} failed ***"
		cat "$log_path"
		exit 1
	fi
}

printf "\n*** Cleaning ReflexCLI ***\n"
run_xcodebuild clean \
	-project "$SCRIPT_DIR/ReflexCLI.xcodeproj" \
	-scheme "Build" \
	-configuration Release \
	-destination "generic/platform=macOS" \
	-derivedDataPath "$DERIVED_DATA_DIR" \
	CONFIGURATION_BUILD_DIR="$OUTPUT_DIR" \
	clean

printf "\n*** Building ReflexCLI ***\n"
run_xcodebuild build \
	-project "$SCRIPT_DIR/ReflexCLI.xcodeproj" \
	-scheme "Build" \
	-configuration Release \
	-destination "generic/platform=macOS" \
	-derivedDataPath "$DERIVED_DATA_DIR" \
	CONFIGURATION_BUILD_DIR="$OUTPUT_DIR" \
	build
