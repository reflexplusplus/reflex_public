#!/bin/bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
VERSION_FILE="$ROOT/version.txt"
INSTALLED_VERSION_FILE="$ROOT/bin/version.txt"

find_reflex_tool() {
	local candidate="$ROOT/bin/tools/macos/reflex"

	if [[ -x "$candidate" ]]; then
		printf '%s\n' "$candidate"
		return 0
	fi

	return 1
}

verify_signed_tool() {
	local tool_path="$1"
	local -a verify_args=(--verify --verbose=4)

	if [[ ! -e "$tool_path" ]]; then
		echo "Expected tool was not installed: $tool_path"
		exit 1
	fi

	if [[ "$tool_path" == *.app ]]; then
		verify_args+=(--deep)
	fi

	if ! codesign "${verify_args[@]}" "$tool_path" >/dev/null 2>&1; then
		echo "Installed tool is not correctly signed: $tool_path"
		exit 1
	fi
}

if [[ ! -f "$VERSION_FILE" ]]; then
	echo "Missing version.txt"
	exit 1
fi

VERSION="$(<"$VERSION_FILE")"

if [[ -f "$INSTALLED_VERSION_FILE" ]]; then
	INSTALLED_VERSION="$(<"$INSTALLED_VERSION_FILE")"

	if [[ "$INSTALLED_VERSION" == "$VERSION" ]] && find_reflex_tool > /dev/null; then
		echo "Reflex SDK $VERSION binaries already installed."
		exit 0
	fi
fi

URL="https://reflexplusplus.b-cdn.net/download/sdk?platform=macos&version=$VERSION"
TMP_BASE="${TMPDIR:-/tmp}"
ZIP="$(mktemp "$TMP_BASE/reflex-sdk-${VERSION}-macos.XXXXXX.zip")"
TEMP_DIR="$(mktemp -d "$TMP_BASE/reflex-sdk-${VERSION}-macos.XXXXXX")"

cleanup_temp_artifacts() {
	chmod -R u+wX "$TEMP_DIR" 2>/dev/null || true
	rm -rf "$TEMP_DIR"
	rm -f "$ZIP"
}

trap cleanup_temp_artifacts EXIT

echo "Downloading Reflex SDK $VERSION binaries..."

curl -L --fail --output "$ZIP" "$URL"

echo "Extracting..."

unzip -q -o "$ZIP" -d "$TEMP_DIR"

if [[ -d "$TEMP_DIR/bin" ]]; then
	mkdir -p "$ROOT/bin"
	cp -R "$TEMP_DIR/bin/." "$ROOT/bin/"
else
	echo "Package does not contain bin folder."
	exit 1
fi

EXTRACT_TOOLS="$ROOT/bin/tools/macos/extract.command"

if [[ -x "$EXTRACT_TOOLS" ]]; then
	"$EXTRACT_TOOLS"
elif [[ -f "$EXTRACT_TOOLS" ]]; then
	bash "$EXTRACT_TOOLS"
fi

echo "Verifying installed macOS tool signatures..."
verify_signed_tool "$ROOT/bin/tools/macos/reflex"
verify_signed_tool "$ROOT/bin/tools/macos/ReflexDocumentation.app"
verify_signed_tool "$ROOT/bin/tools/macos/ReflexProjectCreator.app"
verify_signed_tool "$ROOT/bin/tools/macos/ReflexResourceBuilder.app"

cp "$VERSION_FILE" "$ROOT/bin/version.txt"

echo "Reflex SDK $VERSION binaries installed."
