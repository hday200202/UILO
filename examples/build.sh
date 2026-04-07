#!/usr/bin/env bash
set -e

USAGE="Usage: $0 <target> [cmake-args...]
Targets: sdl_basic, sfml_basic, metal_basic"

if [ $# -lt 1 ]; then
    echo "$USAGE"
    exit 1
fi

TARGET="$1"
shift

case "$TARGET" in
    sdl_basic)   CMAKE_OPT="-DBUILD_SDL_BASIC=ON" ;;
    sfml_basic)  CMAKE_OPT="-DBUILD_SFML_BASIC=ON" ;;
    metal_basic) CMAKE_OPT="-DBUILD_METAL_BASIC=ON" ;;
    *)
        echo "Unknown target: $TARGET"
        echo "$USAGE"
        exit 1
        ;;
esac

BUILD_DIR="build"

cmake -S . -B "$BUILD_DIR" $CMAKE_OPT "$@"
cmake --build "$BUILD_DIR" --target "$TARGET"

echo "Built: bin/$TARGET"
