#!/bin/bash
set -euo pipefail

# Installs the prebuilt Reflex SDK (libs + tools + docs) for macOS from the
# repo's GitHub Release. The git tag is the version; version.txt (baked into the
# distributed tree by CI) selects it, falling back to the latest release.
#
# Environment overrides:
#   REFLEX_GITHUB_REPO   owner/name (default: derived from the git origin remote)
#   REFLEX_GITHUB_TOKEN  required only for the private/licensed repo

ROOT="$(cd "$(dirname "$0")" && pwd)"
PLATFORM="macos"
INSTALLED_VERSION_FILE="$ROOT/bin/version.txt"

# --- resolve version --------------------------------------------------------
if [[ -f "$ROOT/version.txt" ]]; then
	VERSION="$(tr -d '[:space:]' < "$ROOT/version.txt")"
else
	VERSION="latest"
fi

# --- resolve repo -----------------------------------------------------------
REPO="${REFLEX_GITHUB_REPO:-}"
if [[ -z "$REPO" ]]; then
	origin="$(git -C "$ROOT" remote get-url origin 2>/dev/null || true)"
	REPO="$(printf '%s' "$origin" | sed -E 's#\.git$##; s#.*[/:]([^/]+/[^/]+)$#\1#')"
fi
if [[ -z "$REPO" ]]; then
	echo "Cannot determine GitHub repo. Set REFLEX_GITHUB_REPO=owner/name." >&2
	exit 1
fi

TOKEN="${REFLEX_GITHUB_TOKEN:-}"

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

# --- skip if already current ------------------------------------------------
if [[ -f "$INSTALLED_VERSION_FILE" && "$VERSION" != "latest" ]]; then
	if [[ "$(tr -d '[:space:]' < "$INSTALLED_VERSION_FILE")" == "$VERSION" \
		&& -x "$ROOT/bin/tools/$PLATFORM/reflex" ]]; then
		echo "Reflex SDK $VERSION binaries already installed."
		exit 0
	fi
fi

TMP="$(mktemp -d "${TMPDIR:-/tmp}/reflex-sdk.XXXXXX")"
trap 'rm -rf "$TMP"' EXIT

# Download a single release asset and extract it into a destination directory.
download_asset() {
	local name="$1" dest="$2" out="$TMP/$1"

	if [[ -n "$TOKEN" ]]; then
		# Private repo: resolve the asset id from the release JSON, then fetch
		# it via the asset API endpoint (octet-stream).
		if ! command -v python3 >/dev/null 2>&1; then
			echo "REFLEX_GITHUB_TOKEN is set but python3 is required to install from a private repo." >&2
			exit 1
		fi
		local api
		if [[ "$VERSION" == "latest" ]]; then
			api="https://api.github.com/repos/$REPO/releases/latest"
		else
			api="https://api.github.com/repos/$REPO/releases/tags/$VERSION"
		fi
		local id
		id="$(curl -sSL --fail -H "Authorization: Bearer $TOKEN" -H "Accept: application/vnd.github+json" "$api" \
			| python3 -c 'import json,sys; d=json.load(sys.stdin); print(next((a["id"] for a in d.get("assets",[]) if a["name"]==sys.argv[1]), ""))' "$name")"
		if [[ -z "$id" ]]; then
			echo "Release asset not found: $name" >&2
			exit 1
		fi
		curl -sSL --fail -H "Authorization: Bearer $TOKEN" -H "Accept: application/octet-stream" \
			"https://api.github.com/repos/$REPO/releases/assets/$id" -o "$out"
	else
		# Public repo: anonymous download via the stable release URL.
		local url
		if [[ "$VERSION" == "latest" ]]; then
			url="https://github.com/$REPO/releases/latest/download/$name"
		else
			url="https://github.com/$REPO/releases/download/$VERSION/$name"
		fi
		curl -sSL --fail "$url" -o "$out"
	fi

	mkdir -p "$dest"
	unzip -q -o "$out" -d "$dest"
}

create_alias() {
	local launcher_path="$1"
	local target_path="$2"
	mkdir -p "$(dirname "$launcher_path")"
	cat > "$launcher_path" <<EOF
#!/bin/bash
set -euo pipefail
ROOT="\$(cd "\$(dirname "\$0")" && pwd)"
open "$target_path"
EOF
	chmod +x "$launcher_path"
}

echo "Installing Reflex SDK $VERSION ($PLATFORM) from $REPO ..."

download_asset "reflex-libs-$PLATFORM.zip"  "$ROOT/bin/lib"
download_asset "reflex-tools-$PLATFORM.zip" "$ROOT/bin/tools"
download_asset "reflex-docs-$PLATFORM.zip"  "$ROOT/bin/tools"

echo "Verifying installed macOS tool signatures..."
verify_signed_tool "$ROOT/bin/tools/macos/reflex"
verify_signed_tool "$ROOT/bin/tools/macos/ReflexDocumentation.app"
verify_signed_tool "$ROOT/bin/tools/macos/ReflexProjectCreator.app"
verify_signed_tool "$ROOT/bin/tools/macos/ReflexResourceBuilder.app"

# Record the installed version (resolve "latest" from the stamp baked into libs).
RESOLVED="$VERSION"
if [[ "$RESOLVED" == "latest" && -f "$ROOT/bin/lib/$PLATFORM/version.txt" ]]; then
	RESOLVED="$(tr -d '[:space:]' < "$ROOT/bin/lib/$PLATFORM/version.txt")"
fi
mkdir -p "$ROOT/bin"
# Keep the downloaded binaries out of a consumer's git status (the export strips
# bin/ and its .gitignore, so the install output would otherwise show as changes).
printf '*\n' > "$ROOT/bin/.gitignore"
echo "$RESOLVED" > "$INSTALLED_VERSION_FILE"

# --- generate documentation launcher -----------------------------------
create_alias \
	"$ROOT/documentation/ReflexDocumentation.command" \
	"$ROOT/bin/tools/macos/ReflexDocumentation.app"

# --- generate project creator launcher ----------------------------------
create_alias \
	"$ROOT/ReflexProjectCreator.command" \
	"$ROOT/bin/tools/macos/ReflexProjectCreator.app"

echo "Reflex SDK $RESOLVED binaries installed."
