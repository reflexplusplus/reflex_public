#!/bin/bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
VERSION_FILE="$ROOT/version.txt"
INSTALLED_VERSION_FILE="$ROOT/bin/version.txt"

if [[ ! -f "$VERSION_FILE" ]]; then
	echo "Missing version.txt"
	exit 1
fi

VERSION="$(<"$VERSION_FILE")"

if [[ -f "$INSTALLED_VERSION_FILE" ]]; then
	INSTALLED_VERSION="$(<"$INSTALLED_VERSION_FILE")"

	if [[ "$INSTALLED_VERSION" == "$VERSION" ]]; then
		echo "Reflex SDK $VERSION binaries already installed."
		exit 0
	fi
fi

URL="https://reflexplusplus.b-cdn.net/download/sdk?platform=macos&version=$VERSION"
ZIP="${TMPDIR:-/tmp}/reflex-sdk-${VERSION}-macos.zip"
TEMP_DIR="${TMPDIR:-/tmp}/reflex-sdk-${VERSION}-macos"

echo "Downloading Reflex SDK $VERSION binaries..."

curl -L --fail --output "$ZIP" "$URL"

rm -rf "$TEMP_DIR"
mkdir -p "$TEMP_DIR"

echo "Extracting..."

unzip -q -o "$ZIP" -d "$TEMP_DIR"

if [[ -d "$TEMP_DIR/reflex/bin" ]]; then
	mkdir -p "$ROOT/bin"
	cp -R "$TEMP_DIR/reflex/bin/." "$ROOT/bin/"
else
	echo "Package does not contain reflex/bin folder."
	exit 1
fi

cp "$VERSION_FILE" "$ROOT/bin/version.txt"

rm -f "$ZIP"
rm -rf "$TEMP_DIR"

echo "Reflex SDK $VERSION binaries installed."
