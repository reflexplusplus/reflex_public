#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REFLEX_ROOT="$(cd "_REFLEX-PATH_" && pwd)"
TARGET_DIR="$SCRIPT_DIR/agents/reflex"

rm -rf "$TARGET_DIR"
mkdir -p "$TARGET_DIR"

# cp -R "$REFLEX_ROOT/include" "$TARGET_DIR/include"
cp -R "$REFLEX_ROOT/agents/." "$TARGET_DIR/"

printf "Reflex reference files updated.\n"