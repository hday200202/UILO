#!/usr/bin/env bash
# Usage: build.sh [clean] [debug|release] [uilo] [example...]
#
# Tokens (order-independent):
#   clean              Clean the specified targets before building.
#                      With no other targets: wipes the entire build dir.
#                      With targets: cleans only those targets.
#   debug | release    Build type (default: release)
#   uilo               Build the static library target
#   <name>             Build a named example (e.g. containers)
#   (no targets)       Build everything (ninja default target)
#
# Examples:
#   build.sh                          release build of uilo + all examples
#   build.sh clean                    wipe entire release build dir, then rebuild all
#   build.sh clean uilo               clean + rebuild only the library
#   build.sh clean containers         clean + rebuild only containers
#   build.sh clean uilo containers    clean + rebuild library and containers
#   build.sh debug uilo containers    debug build of library + containers

set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"

MODE="release"
CLEAN=0
TARGETS=()

for arg in "$@"; do
    case "$arg" in
        clean)     CLEAN=1 ;;
        debug)     MODE="debug" ;;
        release)   MODE="release" ;;
        -h|--help) sed -n '2,21p' "$0"; exit 0 ;;
        *)         TARGETS+=("$arg") ;;
    esac
done

BUILD_DIR="$ROOT/build/$MODE"

case "$MODE" in
    debug)   CMAKE_BUILD_TYPE="Debug" ;;
    release) CMAKE_BUILD_TYPE="Release" ;;
esac

# Configure (idempotent — cmake skips work when nothing changed)
cmake -S "$ROOT" -B "$BUILD_DIR" \
    -G Ninja \
    -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE"

# Clean
if [[ $CLEAN -eq 1 ]]; then
    if [[ ${#TARGETS[@]} -eq 0 ]]; then
        echo "==> Cleaning all ($MODE)"
        ninja -C "$BUILD_DIR" -t clean
    else
        echo "==> Cleaning: ${TARGETS[*]} ($MODE)"
        ninja -C "$BUILD_DIR" -t clean "${TARGETS[@]}"
    fi
fi

# Build
if [[ ${#TARGETS[@]} -eq 0 ]]; then
    echo "==> Building all ($MODE)"
    ninja -C "$BUILD_DIR"
else
    echo "==> Building: ${TARGETS[*]} ($MODE)"
    ninja -C "$BUILD_DIR" "${TARGETS[@]}"
fi
