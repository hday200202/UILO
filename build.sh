#!/usr/bin/env bash
# Cross-platform build wrapper for UILO (Linux / macOS / WSL / Git-Bash).
# Vendors all third-party dependencies under ext/:
#   - VALO: UILO's renderer and window, cloned into ext/VALO and built as part
#     of UILO; its own `build.sh deps` fetches SDL3's source, the header-only
#     libraries it needs and the Vulkan SDK (loader, validation layers, glslc)
#     into ext/VALO/ext. SDL3 is built from there as part of UILO
#   - stb: headers copied from third_party (for backwards compatibility)
#
# The `wt` argument selects the web backend instead: UILO renders through Wt
# rather than VALO, so it vendors Wt and Boost in their place. VALO is still
# cloned there, for the key and button types UILO's input headers name, but
# none of its dependencies are fetched or built.
#
# Usage:
#   ./build.sh                              # Release static (desktop, VALO)
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

# VALO tracks main: UILO and VALO are developed together, so UILO always
# builds against the latest VALO. Set VALO_REF to build against another ref.
VALO_REPO="https://github.com/hday200202/VALO.git"
VALO_REF="${VALO_REF:-main}"

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
    # Deliberately never touches ext/ -- vendored dependencies survive cleans.
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
    rm -rf "$EXT"/{VALO,wt,boost}
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
# Web backend: Wt + Boost instead of VALO
# ---------------------------------------------------------------------------
# UILO_WT swaps the rendering target, so it needs a different set of vendored
# dependencies -- and none of the GPU ones. Provision those and skip the rest.
if [[ $WT_MODE -eq 1 ]]; then
    clone_if_missing wt "https://github.com/emweb/wt.git" "$WT_TAG"
    # Header-only use: UILO's input headers name VALO's key and button types.
    clone_if_missing VALO "$VALO_REPO" "$VALO_REF"

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

# VALO, then its dependencies: SDL3's source, the header-only libraries and the
# Vulkan SDK, fetched into ext/VALO/ext once. VALO pins the SDL3 version; UILO
# builds it from there. Without curl or network access the Vulkan SDK cannot be
# fetched, and VALO then builds its software renderer only.
clone_if_missing VALO "$VALO_REPO" "$VALO_REF"
echo "[UILO] fetching VALO's dependencies"
bash "$EXT/VALO/build.sh" deps

# A host project that add_subdirectory()s UILO needs ext/ provisioned -- VALO
# cloned and its dependencies fetched -- but not UILO itself built here, since
# it is about to be built inside the host's own tree. Same contract as the web
# path.
if [[ $DEPS_ONLY -eq 1 ]]; then
    echo "[UILO] desktop dependencies ready (ext/VALO)"
    exit 0
fi

case "$(uname -s)" in
    MINGW*|MSYS*|CYGWIN*)
        # -------- Windows desktop: Visual Studio generator --------------------
        # The Visual Studio CMake generator drives MSVC directly (it finds the
        # compiler on its own, no vcvars), so the same ./build.sh works from Git
        # Bash with no .bat/.ps1.
        CMAKE="cmake"
        if ! command -v cmake >/dev/null 2>&1; then
            VSWHERE="/c/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe"
            VS_WIN="$("$VSWHERE" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2>/dev/null || true)"
            [[ -n "$VS_WIN" ]] || { echo "[UILO] error: cmake not on PATH and no Visual Studio found" >&2; exit 1; }
            CMAKE="$(cygpath -u "$VS_WIN")/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe"
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
    Darwin|Linux)
        ;;
    *)
        echo "[UILO] unsupported platform: $(uname -s)" >&2
        exit 1
        ;;
esac

# ---- Linux/macOS: Ninja or Unix Makefiles ---------------------------------
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
