#!/usr/bin/env bash
# Cross-platform build wrapper for UILO (Linux / macOS / WSL / Git-Bash).
# Auto-fetches SDL3 and bgfx via CMake FetchContent when not installed.
#
# Usage:
#   ./build.sh                              # Release static
#   ./build.sh debug                        # Debug static
#   ./build.sh release dynamic              # Release shared
#   ./build.sh debug dynamic                # Debug shared
#   ./build.sh clean release dynamic        # wipe build/ then rebuild
#   UILO_AUTO_FETCH=OFF ./build.sh          # require system SDL3/bgfx
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT_DIR"

MODE="Release"
LINK="static"
DO_CLEAN=0
EXTRA_TARGETS=()

for arg in "$@"; do
    lc_arg=$(printf '%s' "$arg" | tr '[:upper:]' '[:lower:]')
    case "$lc_arg" in
        clean)            DO_CLEAN=1 ;;
        debug)            MODE="Debug" ;;
        release)          MODE="Release" ;;
        static)           LINK="static" ;;
        dynamic|shared)   LINK="dynamic" ;;
        *)                EXTRA_TARGETS+=("$arg") ;;
    esac
done

if [[ "$LINK" == "dynamic" ]]; then
    UILO_SHARED=ON
    LINK_TAG="dynamic"
else
    UILO_SHARED=OFF
    LINK_TAG="static"
fi

BUILD_DIR="build/${MODE}-${LINK_TAG}"
if [[ $DO_CLEAN -eq 1 ]]; then
    echo "[UILO] cleaning $BUILD_DIR"
    rm -rf "$BUILD_DIR"
fi

GENERATOR="Unix Makefiles"
if command -v ninja >/dev/null 2>&1; then
    GENERATOR="Ninja"
fi

AUTO_FETCH="${UILO_AUTO_FETCH:-ON}"

echo "[UILO] configure ($MODE, $LINK_TAG, $GENERATOR, AUTO_FETCH=$AUTO_FETCH)"
cmake -S "$ROOT_DIR" -B "$BUILD_DIR" -G "$GENERATOR" \
    -DCMAKE_BUILD_TYPE="$MODE" \
    -DUILO_AUTO_FETCH="$AUTO_FETCH" \
    -DUILO_SHARED="$UILO_SHARED"

echo "[UILO] build"
if [[ ${#EXTRA_TARGETS[@]} -gt 0 ]]; then
    cmake --build "$BUILD_DIR" --config "$MODE" --parallel --target "${EXTRA_TARGETS[@]}"
else
    cmake --build "$BUILD_DIR" --config "$MODE" --parallel
fi

echo "[UILO] done -> $BUILD_DIR"
