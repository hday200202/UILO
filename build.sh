#!/usr/bin/env bash
# Cross-platform build wrapper for UILO (Linux / macOS / WSL / Git-Bash).
# Vendors all third-party dependencies under ext/:
#   - SDL3: cloned at a pinned release tag, built via CMake
#   - bgfx/bimg/bx: cloned at HEAD, built once with bgfx's GENie+make
#   - stb: headers copied from third_party (for backwards compatibility)
# CMake links the prebuilt static libs and builds SDL3 as part of UILO.
#
# The `wt` argument selects the web backend instead: UILO renders through Wt
# rather than bgfx/SDL3, so it vendors Wt and Boost in their place and none of
# the GPU dependencies are cloned or built at all.
#
# Usage:
#   ./build.sh                              # Release static (desktop, bgfx/SDL3)
#   ./build.sh wt                           # Release static (web, Wt)
#   ./build.sh wt deps                      # only fetch/build ext/, don't build UILO
#                                             (for host projects that add_subdirectory UILO)
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
WT_MODE=0
DEPS_ONLY=0
EXTRA_TARGETS=()

for arg in "$@"; do
    lc_arg=$(printf '%s' "$arg" | tr '[:upper:]' '[:lower:]')
    case "$lc_arg" in
        clean)            DO_CLEAN=1 ;;
        debug)            MODE="Debug" ;;
        release)          MODE="Release" ;;
        static)           LINK="static" ;;
        dynamic|shared)   LINK="dynamic" ;;
        wt|web)           WT_MODE=1 ;;
        deps)             DEPS_ONLY=1 ;;
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

# Pinned versions for the web backend's dependencies.
WT_TAG="4.14.0"                 # https://github.com/emweb/wt tags
BOOST_TAG="boost-1.91.0"        # https://github.com/boostorg/boost tags

if [[ "$(uname -s)" == "Darwin" ]]; then
    JOBS="$(sysctl -n hw.ncpu 2>/dev/null || echo 4)"
else
    JOBS="$(nproc 2>/dev/null || echo 4)"
fi

# Web and desktop builds target different backends from the same tree, so they
# get separate build directories and never invalidate each other's cache.
if [[ $WT_MODE -eq 1 ]]; then
    BUILD_DIR="build/${MODE}-${LINK_TAG}-wt"
else
    BUILD_DIR="build/${MODE}-${LINK_TAG}"
fi
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
    rm -rf "$EXT"/{SDL3,bgfx,bimg,bx,wt,boost}
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

# ---------------------------------------------------------------------------
# Web backend: Wt + Boost instead of SDL3 + bgfx
# ---------------------------------------------------------------------------
# UILO_WT swaps the rendering target, so it needs a different set of vendored
# dependencies -- and none of the GPU ones. Provision those and skip the rest.
if [[ $WT_MODE -eq 1 ]]; then
    clone_if_missing wt "https://github.com/emweb/wt.git" "$WT_TAG"

    # Only the compiled Boost libraries Wt links against, built static so the
    # final executable needs no Boost runtime beside it. The staged name differs
    # per platform, so the "already built?" test globs for either form.
    boost_built() {
        ls "$EXT"/boost/stage/lib/libboost_thread*.a >/dev/null 2>&1 ||
        ls "$EXT"/boost/stage/lib/*boost_thread*.lib >/dev/null 2>&1
    }

    if ! boost_built; then
        if [[ ! -f "$EXT/boost/bootstrap.sh" ]]; then
            echo "[UILO] cloning Boost $BOOST_TAG into ext/ (with submodules; this is large)"
            git clone --depth 1 --branch "$BOOST_TAG" --recurse-submodules \
                --shallow-submodules https://github.com/boostorg/boost.git "$EXT/boost"
        fi
        echo "[UILO] building Boost (b2, $JOBS jobs) -- one time only, takes a while"
        (
            cd "$EXT/boost"
            [[ -x ./b2 ]] || ./bootstrap.sh
            ./b2 headers
            ./b2 --with-thread --with-filesystem --with-program_options \
                 variant=release link=static threading=multi -j"$JOBS" stage
        )
    fi

    # A host project that add_subdirectory()s UILO provisions the deps this way
    # and then builds UILO inside its own tree, so stop before configuring.
    if [[ $DEPS_ONLY -eq 1 ]]; then
        echo "[UILO] web dependencies ready (ext/wt, ext/boost)"
        exit 0
    fi

    GENERATOR="Unix Makefiles"
    command -v ninja >/dev/null 2>&1 && GENERATOR="Ninja"

    echo "[UILO] configure ($MODE, $LINK_TAG, web, $GENERATOR)"
    cmake -S "$ROOT_DIR" -B "$BUILD_DIR" -G "$GENERATOR" \
        -DCMAKE_BUILD_TYPE="$MODE" \
        -DUILO_SHARED="$UILO_SHARED" \
        -DUILO_WT=ON

    echo "[UILO] build"
    if [[ ${#EXTRA_TARGETS[@]} -gt 0 ]]; then
        cmake --build "$BUILD_DIR" --config "$MODE" --parallel --target "${EXTRA_TARGETS[@]}"
    else
        cmake --build "$BUILD_DIR" --config "$MODE" --parallel
    fi

    echo "[UILO] done -> $BUILD_DIR"
    exit 0
fi

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
    -DUILO_SHARED="$UILO_SHARED" \
    -DUILO_WT=OFF

echo "[UILO] build"
if [[ ${#EXTRA_TARGETS[@]} -gt 0 ]]; then
    cmake --build "$BUILD_DIR" --config "$MODE" --parallel --target "${EXTRA_TARGETS[@]}"
else
    cmake --build "$BUILD_DIR" --config "$MODE" --parallel
fi

echo "[UILO] done -> $BUILD_DIR"
