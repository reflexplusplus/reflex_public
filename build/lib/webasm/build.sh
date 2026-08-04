#!/bin/sh
# Either set EMROOT to your emscripten root folder, or ensure emscripten is setup in your path
# You can trip the emsdk/emsdk_env.sh via or set in your .bash_profile or similar
#  source "...path_to_emscripten.../emsdk/emsdk_env.sh"
set -e

if [ "$1" != "Debug" ] && [ "$1" != "Release" ]; then
	echo "Usage: build.sh <Debug|Release>\n\nBuilds this project (for webasm); arg is case sensitive.\n" >&2
	exit -1
fi

cd "$(dirname "$0")"

CONFIG=$1
EMROOT=$(dirname $(which emcc))
TOOLCHAIN="-DCMAKE_TOOLCHAIN_FILE=$EMSDK/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake"

# ========= Build =========
mkdir -p build/$CONFIG && cd build/$CONFIG

echo "➡️ Configuring…"
cmake $TOOLCHAIN -S ../../ -B . -DCMAKE_BUILD_TYPE=$CONFIG

echo "➡️ Building…"
cmake --build . --target AllLibraries
