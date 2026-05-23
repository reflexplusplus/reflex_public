#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
TOOLS_DIR="$SCRIPT_DIR"

if [[ ! -d "$TOOLS_DIR" ]]; then
  echo "Tools directory not found: $TOOLS_DIR" >&2
  exit 1
fi

found_any=0

while IFS= read -r -d '' dmg; do
  found_any=1
  echo "Extracting $(basename "$dmg")..."

  mount_point=""

  cleanup() {
    if [[ -n "$mount_point" && -d "$mount_point" ]]; then
      hdiutil detach "$mount_point" -quiet || true
    fi
  }

  trap cleanup EXIT

  if ! mount_point="$(hdiutil attach -readonly -nobrowse "$dmg" | tail -n 1 | cut -f 3-)"; then
    echo "Failed to mount $dmg" >&2
    exit 1
  fi

  if [[ -z "$mount_point" || ! -d "$mount_point" ]]; then
    echo "Failed to determine mount point for $dmg" >&2
    exit 1
  fi

  copied_any=0

  while IFS= read -r item; do
    name="$(basename "$item")"
    destination="$TOOLS_DIR/$name"

    case "$name" in
      ".Trashes"|".fseventsd"|".Spotlight-V100"|".background"|".DS_Store")
        continue
        ;;
    esac

    copied_any=1

    if [[ -e "$destination" ]]; then
      rm -rf "$destination"
    fi

    ditto "$item" "$destination"
    echo "  wrote $name"
  done < <(find "$mount_point" -mindepth 1 -maxdepth 1 -print)

  if [[ "$copied_any" -eq 0 ]]; then
    echo "No extractable items found in $dmg" >&2
    exit 1
  fi

  hdiutil detach "$mount_point" -quiet
  trap - EXIT
  mount_point=""
  echo "Finished $(basename "$dmg")"
done < <(find "$TOOLS_DIR" -maxdepth 1 -type f -name '*.dmg' -print0)

if [[ "$found_any" -eq 0 ]]; then
  echo "No DMG files found in $TOOLS_DIR"
fi
