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
#   ./build.sh deps                         # only fetch/build ext/, don't build UILO
#   ./build.sh wt deps                      # the same for the web backend
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

# Pinned versions. bgfx publishes no tags, so those three are commit SHAs; see
# the note above the clone calls for why leaving them floating breaks the build.
BX_REF="efdef94b9486509e85b906170020f4c883d3eed5"    # 2026-07-03
BIMG_REF="3b3dc2b7081fc687bce3a73699b0b8db470f1afc"  # 2026-06-23
BGFX_REF="aa176c7763e9dcf90c8ac2345f1b984a12fbefbe"  # 2026-07-04

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

# Clones a dependency at a pinned ref, which may be a tag, a branch or a bare
# commit SHA. --branch takes the first two but not the third, so a SHA falls
# back to fetching that one commit directly -- GitHub serves any commit this
# way, and it is still a one-commit download.
clone_if_missing() {
    local name="$1" url="$2" ref="${3:-}"
    [[ -d "$EXT/$name" ]] && return 0

    echo "[UILO] cloning $name into ext/"
    if [[ -z "$ref" ]]; then
        git clone --depth 1 "$url" "$EXT/$name"
        return
    fi
    if git clone --depth 1 --branch "$ref" "$url" "$EXT/$name" 2>/dev/null; then
        return
    fi

    rm -rf "$EXT/$name"
    git init -q "$EXT/$name"
    git -C "$EXT/$name" remote add origin "$url"
    git -C "$EXT/$name" fetch -q --depth 1 origin "$ref"
    git -C "$EXT/$name" checkout -q FETCH_HEAD
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
        case "$(uname -s)" in
            MINGW*|MSYS*|CYGWIN*)
                # Windows: build with MSVC. b2's Unix bootstrap.sh can't drive
                # MSVC, so run bootstrap.bat + b2 inside a vcvars environment via
                # PowerShell -> cmd. Several MSYS/cmd quirks are handled: the batch
                # needs CRLF endings; NoDefaultCurrentDirectoryInExePath must be
                # cleared (MSYS sets it, breaking bootstrap's internal calls);
                # vcvars changes directory so we cd back; and /c is passed to cmd
                # via PowerShell because `cmd //c` through Git Bash is unreliable.
                VSWHERE="/c/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe"
                [[ -f "$VSWHERE" ]] || { echo "[UILO] error: vswhere not found; install Visual Studio with the C++ workload" >&2; exit 1; }
                VS_WIN="$("$VSWHERE" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath)"
                [[ -n "$VS_WIN" ]] || { echo "[UILO] error: no VS C++ x64 toolset found (install 'Desktop development with C++')" >&2; exit 1; }
                VCVARS="$VS_WIN\\VC\\Auxiliary\\Build\\vcvars64.bat"
                BOOST_ABS_BS="$(cd "$EXT/boost" && pwd -W | sed 's#/#\\#g')"
                cat > "$EXT/boost/_build_boost.bat" <<BAT
@echo off
set "NoDefaultCurrentDirectoryInExePath="
call "$VCVARS" || exit /b 1
cd /d "$BOOST_ABS_BS" || exit /b 1
if not exist b2.exe call bootstrap.bat
if not exist b2.exe exit /b 1
b2 headers || exit /b 1
b2 --with-thread --with-filesystem --with-program_options toolset=msvc runtime-link=shared address-model=64 variant=release link=static threading=multi -j$JOBS stage || exit /b 1
BAT
                sed -i 's/$/\r/' "$EXT/boost/_build_boost.bat"
                powershell -NoProfile -Command "cmd /c '${BOOST_ABS_BS}\\_build_boost.bat'"
                ;;
            *)
                # Linux/macOS: bootstrap + b2 with an explicit toolset. b2's
                # auto-guess fails with a cryptic dump when no compiler is on
                # PATH, so detect one and give an actionable error instead.
                if [[ "$(uname -s)" == "Darwin" ]]; then
                    command -v clang++ >/dev/null 2>&1 && TS=clang || TS=""
                elif command -v g++ >/dev/null 2>&1; then
                    TS=gcc
                elif command -v clang++ >/dev/null 2>&1; then
                    TS=clang
                else
                    TS=""
                fi
                if [[ -z "$TS" ]]; then
                    echo "[UILO] error: no C++ compiler found (need g++ or clang++)." >&2
                    echo "       Linux (Debian/Ubuntu): sudo apt install build-essential" >&2
                    echo "       Linux (Fedora): sudo dnf install gcc-c++   (Arch): sudo pacman -S base-devel" >&2
                    echo "       macOS: xcode-select --install" >&2
                    exit 1
                fi
                echo "[UILO] toolset: $TS"
                (
                    cd "$EXT/boost"
                    [[ -x ./b2 ]] || ./bootstrap.sh --with-toolset="$TS"
                    ./b2 headers
                    ./b2 toolset="$TS" --with-thread --with-filesystem --with-program_options \
                         variant=release link=static threading=multi -j"$JOBS" stage
                )
                ;;
        esac
    fi

    # A host project that add_subdirectory()s UILO provisions the deps this way
    # and then builds UILO inside its own tree, so stop before configuring.
    if [[ $DEPS_ONLY -eq 1 ]]; then
        echo "[UILO] web dependencies ready (ext/wt, ext/boost)"
        exit 0
    fi

    # Configure with a generator that fits the platform. On Windows the Visual
    # Studio generator drives MSVC directly (no vcvars/nmake needed), so the same
    # ./build.sh works there via Git Bash -- no .bat/.ps1. Elsewhere prefer Ninja,
    # then Unix Makefiles.
    CMAKE="cmake"
    case "$(uname -s)" in
        MINGW*|MSYS*|CYGWIN*)
            # cmake may not be on PATH on Windows; fall back to the VS-bundled one.
            if ! command -v cmake >/dev/null 2>&1; then
                VSWHERE="/c/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe"
                VS_WIN="$("$VSWHERE" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2>/dev/null || true)"
                [[ -n "$VS_WIN" ]] || { echo "[UILO] error: cmake not on PATH and no Visual Studio found" >&2; exit 1; }
                CMAKE="$(cygpath -u "$VS_WIN")/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe"
            fi
            echo "[UILO] configure ($MODE, $LINK_TAG, web, Visual Studio)"
            "$CMAKE" -S "$ROOT_DIR" -B "$BUILD_DIR" -A x64 \
                -DCMAKE_BUILD_TYPE="$MODE" \
                -DUILO_SHARED="$UILO_SHARED" \
                -DUILO_WT=ON
            ;;
        *)
            GENERATOR="Unix Makefiles"
            command -v ninja >/dev/null 2>&1 && GENERATOR="Ninja"
            echo "[UILO] configure ($MODE, $LINK_TAG, web, $GENERATOR)"
            "$CMAKE" -S "$ROOT_DIR" -B "$BUILD_DIR" -G "$GENERATOR" \
                -DCMAKE_BUILD_TYPE="$MODE" \
                -DUILO_SHARED="$UILO_SHARED" \
                -DUILO_WT=ON
            ;;
    esac

    echo "[UILO] build"
    if [[ ${#EXTRA_TARGETS[@]} -gt 0 ]]; then
        "$CMAKE" --build "$BUILD_DIR" --config "$MODE" --parallel --target "${EXTRA_TARGETS[@]}"
    else
        "$CMAKE" --build "$BUILD_DIR" --config "$MODE" --parallel
    fi

    echo "[UILO] done -> $BUILD_DIR"
    exit 0
fi

# SDL3: pinned release tag for reproducible builds
clone_if_missing SDL3 "https://github.com/libsdl-org/SDL.git" "release-3.2.10"

# bgfx: bx/bimg/bgfx MUST be sibling dirs (bgfx's build references ../bx and ../bimg)
#
# Pinned, and they have to stay pinned. bgfx has no release tags and its shaderc
# drops shader profiles over time -- the GLSL profile this project asks for
# (120, see _UILO_SHADER_PROFILES in CMakeLists.txt) was removed upstream, so a
# clone at HEAD fails every shader with "Unknown profile: 120" while an older
# checkout builds. Bumping these three is a deliberate step that means checking
# the profile list still matches.
clone_if_missing bx   "https://github.com/bkaradzic/bx.git"   "$BX_REF"
clone_if_missing bimg "https://github.com/bkaradzic/bimg.git" "$BIMG_REF"
clone_if_missing bgfx "https://github.com/bkaradzic/bgfx.git" "$BGFX_REF"

case "$(uname -s)" in
    MINGW*|MSYS*|CYGWIN*)
        # -------- Windows desktop: GENie + MSBuild for bgfx, VS gen for UILO --
        # Same single-entrypoint story as Linux/macOS -- no .bat/.ps1 to run by
        # hand. bgfx has no Unix-make target on Windows, so it's built from its
        # generated VS solution; the Visual Studio CMake generator then drives
        # MSVC for UILO itself (it finds the compiler on its own, no vcvars).
        VSWHERE="/c/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe"
        [[ -f "$VSWHERE" ]] || { echo "[UILO] error: vswhere not found; install Visual Studio with the C++ workload" >&2; exit 1; }
        VS_WIN="$("$VSWHERE" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2>/dev/null || true)"
        [[ -n "$VS_WIN" ]] || { echo "[UILO] error: no VS C++ x64 toolset found (install 'Desktop development with C++')" >&2; exit 1; }
        VS_DIR="$(cygpath -u "$VS_WIN")"
        VCVARS="$VS_WIN\\VC\\Auxiliary\\Build\\vcvars64.bat"

        CMAKE="cmake"
        command -v cmake >/dev/null 2>&1 || CMAKE="$VS_DIR/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe"

        # Retarget bgfx's vs2022/v143 projects onto whatever MSVC toolset +
        # Windows SDK this machine actually has (e.g. v145), so a machine without
        # the exact vs2022 toolset still builds bgfx.
        TOOLSET="$(ls -d "$VS_DIR"/MSBuild/Microsoft/VC/*/Platforms/x64/PlatformToolsets/v* 2>/dev/null | sed 's#.*/##' | grep -E '^v[0-9]+$' | sort -V | tail -1 || true)"
        [[ -n "$TOOLSET" ]] || { echo "[UILO] error: no MSVC platform toolset found" >&2; exit 1; }
        WINSDK="$(ls -d "/c/Program Files (x86)/Windows Kits/10/bin/10."* 2>/dev/null | sed 's#.*/##' | sort -V | tail -1 || true)"
        WINSDK_ARG=""
        [[ -n "$WINSDK" ]] && WINSDK_ARG="/p:WindowsTargetPlatformVersion=$WINSDK"
        echo "[UILO] toolset=$TOOLSET winsdk=${WINSDK:-<default>}"

        # One-time bgfx build (Release x64, +tools for shaderc), gated on the
        # output lib. Driven through a generated .bat run via PowerShell -> cmd
        # so genie/msbuild get a proper vcvars environment (same MSYS/cmd quirk
        # handling as the Boost build).
        if [[ ! -f "$EXT/bgfx/.build/win64_vs2022/bin/bgfxRelease.lib" ]]; then
            echo "[UILO] building bgfx (win64_vs2022, Release) -- one time only"
            BGFX_ABS_BS="$(cd "$EXT/bgfx" && pwd -W | sed 's#/#\\#g')"
            cat > "$EXT/bgfx/_build_bgfx.bat" <<BAT
@echo off
set "NoDefaultCurrentDirectoryInExePath="
call "$VCVARS" || exit /b 1
cd /d "$BGFX_ABS_BS" || exit /b 1
..\bx\tools\bin\windows\genie.exe --with-tools --file=scripts\genie.lua vs2022 || exit /b 1
msbuild .build\projects\vs2022\bgfx.sln /p:Configuration=Release /p:Platform=x64 /p:PlatformToolset=$TOOLSET $WINSDK_ARG /m || exit /b 1
BAT
            sed -i 's/$/\r/' "$EXT/bgfx/_build_bgfx.bat"
            powershell -NoProfile -Command "cmd /c '${BGFX_ABS_BS}\\_build_bgfx.bat'"
        fi

        echo "[UILO] configure ($MODE, $LINK_TAG, desktop, Visual Studio)"
        "$CMAKE" -S "$ROOT_DIR" -B "$BUILD_DIR" -A x64 \
            -DCMAKE_BUILD_TYPE="$MODE" \
            -DUILO_SHARED="$UILO_SHARED" \
            -DUILO_WT=OFF
        echo "[UILO] build"
        if [[ ${#EXTRA_TARGETS[@]} -gt 0 ]]; then
            "$CMAKE" --build "$BUILD_DIR" --config "$MODE" --parallel --target "${EXTRA_TARGETS[@]}"
        else
            "$CMAKE" --build "$BUILD_DIR" --config "$MODE" --parallel
        fi
        echo "[UILO] done -> $BUILD_DIR"
        exit 0
        ;;
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
        exit 1
        ;;
esac

# ---- Linux/macOS: make for bgfx, Ninja/Unix Makefiles for UILO ------------
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

# A host project that add_subdirectory()s UILO needs ext/ provisioned -- SDL3
# cloned, bgfx and shaderc built -- but not UILO itself built here, since it is
# about to be built inside the host's own tree. Same contract as the web path.
if [[ $DEPS_ONLY -eq 1 ]]; then
    echo "[UILO] desktop dependencies ready (ext/SDL3, ext/bgfx)"
    exit 0
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
