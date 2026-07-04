#!/usr/bin/env bash
# Cross-platform build wrapper for UILO (Linux / macOS / WSL / Git-Bash).
# Vendors bgfx: clones bx/bimg/bgfx under ext/ and builds them ONCE with
# bgfx's own GENie+make build; CMake links the prebuilt static libs.
# SDL3 is auto-fetched via CMake FetchContent when not installed.
#
# Usage:
#   ./build.sh                              # Release static
#   ./build.sh debug                        # Debug static
#   ./build.sh release dynamic              # Release shared
#   ./build.sh debug dynamic                # Debug shared
#   ./build.sh clean release dynamic        # wipe build/ then rebuild
#                                             (never touches ext/)
#   UILO_AUTO_FETCH=OFF ./build.sh          # require system SDL3
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
    # Deliberately never touches ext/ -- the one-time bgfx build survives cleans.
    echo "[UILO] cleaning $BUILD_DIR"
    rm -rf "$BUILD_DIR"
fi

# ---------------------------------------------------------------------------
# Vendored bgfx: bx/bimg/bgfx MUST be sibling dirs under ext/ (bgfx's build
# references ../bx and ../bimg). Clone all three together -- they version-lock
# against each other. GENie needs no install; bx ships prebuilt binaries at
# bx/tools/bin/<os>/genie, which bgfx's makefile uses automatically.
# ---------------------------------------------------------------------------
EXT="$ROOT_DIR/ext"
mkdir -p "$EXT"

clone_if_missing() {
    local name="$1" url="$2"
    if [[ ! -d "$EXT/$name" ]]; then
        echo "[UILO] cloning $name into ext/"
        git clone --depth 1 "$url" "$EXT/$name"
    fi
}
clone_if_missing bx   "https://github.com/bkaradzic/bx.git"
clone_if_missing bimg "https://github.com/bkaradzic/bimg.git"
clone_if_missing bgfx "https://github.com/bkaradzic/bgfx.git"

case "$(uname -s)" in
    Darwin)
        if [[ "$(uname -m)" == "arm64" ]]; then
            BGFX_TARGET="osx-arm64-release"
            BGFX_PLATFORM_DIR="osx-arm64"
        else
            BGFX_TARGET="osx-x64-release"
            BGFX_PLATFORM_DIR="osx-x64"
        fi
        ;;
    Linux)
        BGFX_TARGET="linux-gcc-release64"
        BGFX_PLATFORM_DIR="linux64_gcc"
        ;;
    *)
        echo "[UILO] unsupported platform for vendored bgfx: $(uname -s)" >&2
        echo "[UILO] on Windows use build.bat / build.ps1" >&2
        exit 1
        ;;
esac
BGFX_BIN_DIR="$EXT/bgfx/.build/$BGFX_PLATFORM_DIR/bin"

# One-time build (release libs + shaderc via --with-tools), gated on the
# output archive. Always Release, even for Debug UILO builds -- you debug
# UILO, not bgfx.
if [[ ! -f "$BGFX_BIN_DIR/libbgfxRelease.a" ]]; then
    echo "[UILO] building bgfx ($BGFX_TARGET) -- one time only"
    make -C "$EXT/bgfx" "$BGFX_TARGET"
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
