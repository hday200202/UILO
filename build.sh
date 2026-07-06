#!/usr/bin/env bash
# Cross-platform build wrapper for UILO (Linux / macOS / WSL / Git-Bash).
# Vendors all third-party dependencies under ext/:
#   - SDL3: cloned at a pinned release tag, built via CMake
#   - bgfx/bimg/bx: cloned at HEAD, built once with bgfx's GENie+make
#   - stb: headers copied from third_party (for backwards compatibility)
# CMake links the prebuilt static libs and builds SDL3 as part of UILO.
#
# Usage:
#   ./build.sh                              # Release static
#   ./build.sh debug                        # Debug static
#   ./build.sh release dynamic              # Release shared
#   ./build.sh debug dynamic                # Debug shared
#   ./build.sh clean release dynamic        # wipe build/ then rebuild
#                                             (never touches ext/)
# Env:
#   UILO_CLEAN_EXT=1 ./build.sh             # also wipe ext/ (forces re-clone/rebuild)
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
# Vendored dependencies under ext/
# ---------------------------------------------------------------------------
EXT="$ROOT_DIR/ext"
mkdir -p "$EXT"

UILO_CLEAN_EXT="${UILO_CLEAN_EXT:-0}"
if [[ $UILO_CLEAN_EXT -eq 1 ]]; then
    echo "[UILO] cleaning ext/ (UILO_CLEAN_EXT=1)"
    rm -rf "$EXT"/{SDL3,bgfx,bimg,bx}
    mkdir -p "$EXT"
fi

clone_if_missing() {
    local name="$1" url="$2" tag="${3:-}"
    if [[ ! -d "$EXT/$name" ]]; then
        echo "[UILO] cloning $name into ext/"
        if [[ -n "$tag" ]]; then
            git clone --depth 1 --branch "$tag" "$url" "$EXT/$name"
        else
            git clone --depth 1 "$url" "$EXT/$name"
        fi
    fi
}

# SDL3: pinned release tag for reproducible builds
clone_if_missing SDL3 "https://github.com/libsdl-org/SDL.git" "release-3.2.10"

# bgfx: bx/bimg/bgfx MUST be sibling dirs (bgfx's build references ../bx and ../bimg)
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
        echo "[UILO] unsupported platform: $(uname -s)" >&2
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
# Ensure shaderc is available (needed for shader compilation)
if [[ ! -f "$BGFX_BIN_DIR/shadercRelease" ]]; then
    echo "[UILO] building bgfx tools -- one time only"
    make -C "$EXT/bgfx" "$BGFX_TARGET" TOOLS=1 2>/dev/null || true
fi

GENERATOR="Unix Makefiles"
if command -v ninja >/dev/null 2>&1; then
    GENERATOR="Ninja"
fi

echo "[UILO] configure ($MODE, $LINK_TAG, $GENERATOR)"
cmake -S "$ROOT_DIR" -B "$BUILD_DIR" -G "$GENERATOR" \
    -DCMAKE_BUILD_TYPE="$MODE" \
    -DUILO_SHARED="$UILO_SHARED"

echo "[UILO] build"
if [[ ${#EXTRA_TARGETS[@]} -gt 0 ]]; then
    cmake --build "$BUILD_DIR" --config "$MODE" --parallel --target "${EXTRA_TARGETS[@]}"
else
    cmake --build "$BUILD_DIR" --config "$MODE" --parallel
fi

echo "[UILO] done -> $BUILD_DIR"
