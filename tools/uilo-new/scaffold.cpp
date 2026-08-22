#include "scaffold.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <system_error>

namespace fs = std::filesystem;

namespace uilonew {

namespace {

/*
    substitute(std::string text, const std::string& token, const std::string& value):
    - Params:   std::string text, const std::string& token, const std::string&
                value
    - Returns:  std::string
    - Desc:     Replaces every occurrence of a template token. The templates use
                @NAME@-style tokens because nothing in CMake, shell or C++ uses
                that spelling, so a template is still valid source to read.
*/
std::string substitute(std::string text, const std::string& token, const std::string& value) {
    for (size_t at = text.find(token); at != std::string::npos; at = text.find(token, at + value.size()))
        text.replace(at, token.size(), value);
    return text;
}


/*
    writeFile(const fs::path& path, const std::string& content, std::string& error):
    - Params:   const fs::path& path, const std::string& content, std::string&
                error
    - Returns:  bool
    - Desc:     Writes one file, creating its parent directories. Binary mode so
                the generated build script keeps LF endings on Windows, where a
                CRLF shebang line stops the script running under Git Bash.
*/
bool writeFile(const fs::path& path, const std::string& content, std::string& error) {
    std::error_code ec;
    fs::create_directories(path.parent_path(), ec);
    if (ec) { error = "could not create " + path.parent_path().string() + ": " + ec.message(); return false; }

    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) { error = "could not write " + path.string(); return false; }
    out.write(content.data(), static_cast<std::streamsize>(content.size()));
    if (!out) { error = "could not write " + path.string(); return false; }
    return true;
}

} // namespace


const char* templateName(Template t) {
    switch (t) {
        case Template::Panels: return "panels";
        case Template::Minimal:
        default:               return "minimal";
    }
}


Template templateFromName(const std::string& name, bool* ok) {
    if (ok) *ok = true;
    if (name == "minimal") return Template::Minimal;
    if (name == "panels")  return Template::Panels;
    if (ok) *ok = false;
    return Template::Minimal;
}


/*
    validName(const std::string& name, std::string* why):
    - Params:   const std::string& name, std::string* why
    - Returns:  bool
    - Desc:     Whether the name works as both a CMake target and a file name.
                Letters, digits, underscore and dash, not starting with a digit --
                the intersection of what CMake, every shell and every filesystem
                here will take without quoting.
*/
bool validName(const std::string& name, std::string* why) {
    auto fail = [&](const char* reason) { if (why) *why = reason; return false; };

    if (name.empty())                       return fail("the project name is empty");
    if (std::isdigit(static_cast<unsigned char>(name[0])))
                                            return fail("the project name cannot start with a digit");
    for (char c : name) {
        const bool okChar = std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-';
        if (!okChar) return fail("the project name may only contain letters, digits, '_' and '-'");
    }
    if (why) why->clear();
    return true;
}


std::string validate(const Options& options) {
    if (options.path.empty()) return "no path given";

    std::string name = options.name.empty() ? options.path.filename().string() : options.name;
    std::string why;
    if (!validName(name, &why)) return why;

    std::error_code ec;
    if (fs::exists(options.path, ec) && !options.force) {
        if (!fs::is_directory(options.path, ec)) return options.path.string() + " exists and is not a directory";
        if (!fs::is_empty(options.path, ec))     return options.path.string() + " is not empty (pass --force to write into it)";
    }
    return {};
}


/* ------------------------------------------------------------------------- */
/* Templates. @NAME@, @REPO@ and @REF@ are substituted; everything else is    */
/* written out verbatim.                                                      */
/* ------------------------------------------------------------------------- */

const char* kBuildScript = R"SH(#!/usr/bin/env bash
# Build script for @NAME@.
#
# It provisions everything the project needs and then builds it:
#   - checks for git, cmake, python3, a C++ compiler and (on Linux) the X11 and
#     Wayland development packages SDL3 links against, and offers to install
#     whatever is missing with the platform's package manager
#   - clones UILO into ext/UILO at a pinned ref, if it is not already there
#   - has UILO provision its own vendored dependencies (SDL3, bgfx, shaders)
#   - configures and builds this project, with the binary landing in bin/
#
# Usage:
#   ./build.sh                # Release
#   ./build.sh debug          # Debug
#   ./build.sh clean          # wipe build/ first (leaves ext/ alone)
#   ./build.sh run            # build, then run bin/@NAME@
#   ./build.sh --yes          # install missing dependencies without asking
#   ./build.sh --no-install   # never install; just report what is missing
#
# Windows: run this from Git Bash. UILO drives MSVC through CMake's Visual
# Studio generator, so no separate .bat or .ps1 is needed.
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT_DIR"

UILO_REPO="@REPO@"
UILO_REF="@REF@"

MODE="Release"
DO_CLEAN=0
DO_RUN=0
ASSUME_YES=0
NO_INSTALL=0

for arg in "$@"; do
    case "$(printf '%s' "$arg" | tr '[:upper:]' '[:lower:]')" in
        debug)        MODE="Debug" ;;
        release)      MODE="Release" ;;
        clean)        DO_CLEAN=1 ;;
        run)          DO_RUN=1 ;;
        -y|--yes)     ASSUME_YES=1 ;;
        --no-install) NO_INSTALL=1 ;;
        -h|--help)    sed -n '2,27p' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
        *)            echo "unknown argument: $arg" >&2; exit 2 ;;
    esac
done

say()  { printf '[@NAME@] %s\n' "$*"; }
die()  { printf '[@NAME@] error: %s\n' "$*" >&2; exit 1; }

# ---- platform and package manager ----------------------------------------
OS="$(uname -s)"
PM=""
PM_INSTALL=""
case "$OS" in
    Darwin)
        if command -v brew >/dev/null 2>&1; then PM="brew"; PM_INSTALL="brew install"; fi
        ;;
    Linux)
        if   command -v apt-get >/dev/null 2>&1; then PM="apt";    PM_INSTALL="sudo apt-get install -y"
        elif command -v dnf     >/dev/null 2>&1; then PM="dnf";    PM_INSTALL="sudo dnf install -y"
        elif command -v pacman  >/dev/null 2>&1; then PM="pacman"; PM_INSTALL="sudo pacman -S --noconfirm"
        elif command -v zypper  >/dev/null 2>&1; then PM="zypper"; PM_INSTALL="sudo zypper install -y"
        fi
        ;;
    MINGW*|MSYS*|CYGWIN*)
        if command -v winget >/dev/null 2>&1; then PM="winget"; PM_INSTALL="winget install --accept-package-agreements --accept-source-agreements -e --id"; fi
        ;;
esac

# Asks before installing, unless --yes was given. Returns non-zero when the
# answer is no, so the caller can carry on and report at the end.
confirm() {
    [[ $ASSUME_YES -eq 1 ]] && return 0
    [[ -t 0 ]] || return 1
    printf '[@NAME@] install %s with %s? [y/N] ' "$1" "$PM"
    read -r reply
    [[ "$reply" == "y" || "$reply" == "Y" ]]
}

MISSING=()

# require <command> <brew pkg> <apt pkg> <dnf pkg> <pacman pkg> <winget id>
require() {
    local cmd="$1"; shift
    command -v "$cmd" >/dev/null 2>&1 && return 0

    local pkg=""
    case "$PM" in
        brew)   pkg="$1" ;;
        apt)    pkg="$2" ;;
        dnf)    pkg="$3" ;;
        pacman) pkg="$4" ;;
        winget) pkg="$5" ;;
    esac

    if [[ -z "$PM" || -z "$pkg" || $NO_INSTALL -eq 1 ]]; then
        MISSING+=("$cmd")
        return 0
    fi
    if confirm "$cmd"; then
        say "installing $cmd"
        # shellcheck disable=SC2086
        $PM_INSTALL $pkg || MISSING+=("$cmd")
    else
        MISSING+=("$cmd")
    fi
}

say "checking dependencies"
require git     git    git             git             git       Git.Git
require cmake   cmake  cmake           cmake           cmake     Kitware.CMake
require python3 python python3         python3         python    Python.Python.3.12

# A C++ compiler. Apple ships one behind the command line tools; elsewhere it
# comes from the platform's build-essential equivalent.
if ! command -v c++ >/dev/null 2>&1 && ! command -v cl.exe >/dev/null 2>&1; then
    case "$OS" in
        Darwin) say "no compiler found -- run: xcode-select --install"; MISSING+=("c++") ;;
        Linux)
            case "$PM" in
                apt)    require_pkg="build-essential" ;;
                dnf)    require_pkg="gcc-c++ make" ;;
                pacman) require_pkg="base-devel" ;;
                *)      require_pkg="" ;;
            esac
            if [[ -n "${require_pkg:-}" ]] && confirm "a C++ compiler"; then
                # shellcheck disable=SC2086
                $PM_INSTALL $require_pkg || MISSING+=("c++")
            else
                MISSING+=("c++")
            fi
            ;;
        *) say "no compiler found -- install the Visual Studio 2022 build tools"; MISSING+=("c++") ;;
    esac
fi

# SDL3 needs X11 and Wayland headers on Linux; nothing to do elsewhere.
if [[ "$OS" == "Linux" && $NO_INSTALL -eq 0 ]]; then
    NEED_SDL_DEV=0
    [[ -f /usr/include/X11/Xlib.h ]] || NEED_SDL_DEV=1
    if [[ $NEED_SDL_DEV -eq 1 ]]; then
        case "$PM" in
            apt)    sdl_pkgs="libx11-dev libxext-dev libxrandr-dev libxi-dev libxcursor-dev libxfixes-dev libxss-dev libwayland-dev libxkbcommon-dev libgl1-mesa-dev" ;;
            dnf)    sdl_pkgs="libX11-devel libXext-devel libXrandr-devel libXi-devel libXcursor-devel libXfixes-devel wayland-devel libxkbcommon-devel mesa-libGL-devel" ;;
            pacman) sdl_pkgs="libx11 libxext libxrandr libxi libxcursor libxfixes wayland libxkbcommon mesa" ;;
            *)      sdl_pkgs="" ;;
        esac
        if [[ -n "$sdl_pkgs" ]] && confirm "SDL3's X11/Wayland development packages"; then
            # shellcheck disable=SC2086
            $PM_INSTALL $sdl_pkgs || say "could not install the SDL3 development packages -- continuing"
        fi
    fi
fi

if [[ ${#MISSING[@]} -gt 0 ]]; then
    say "missing: ${MISSING[*]}"
    [[ -z "$PM" ]] && say "no supported package manager found; install them by hand"
    die "install the tools above, then run ./build.sh again"
fi

# ---- UILO ----------------------------------------------------------------
if [[ ! -d ext/UILO/.git ]]; then
    say "fetching UILO ($UILO_REF)"
    mkdir -p ext
    git clone --depth 1 --branch "$UILO_REF" "$UILO_REPO" ext/UILO \
        || git clone "$UILO_REPO" ext/UILO
    # A tag or SHA that --branch could not take is checked out here instead.
    ( cd ext/UILO && git rev-parse --verify --quiet "$UILO_REF^{commit}" >/dev/null \
        && git checkout --quiet "$UILO_REF" ) || true
else
    say "UILO already present in ext/UILO"
fi

# UILO vendors SDL3 and bgfx under its own ext/ and compiles the shaders. This
# provisions them without building UILO itself, which is about to be built as
# part of this project.
say "provisioning UILO's dependencies (first run takes a while)"
bash ext/UILO/build.sh deps

# ---- this project --------------------------------------------------------
BUILD_DIR="build/$MODE"
[[ $DO_CLEAN -eq 1 ]] && rm -rf "$BUILD_DIR"

GENERATOR="Unix Makefiles"
command -v ninja >/dev/null 2>&1 && GENERATOR="Ninja"
case "$OS" in MINGW*|MSYS*|CYGWIN*) GENERATOR="" ;; esac

say "configure ($MODE)"
if [[ -n "$GENERATOR" ]]; then
    cmake -S . -B "$BUILD_DIR" -G "$GENERATOR" -DCMAKE_BUILD_TYPE="$MODE"
else
    cmake -S . -B "$BUILD_DIR" -A x64 -DCMAKE_BUILD_TYPE="$MODE"
fi

say "build"
cmake --build "$BUILD_DIR" --config "$MODE" --parallel

say "done -> bin/@NAME@"
[[ $DO_RUN -eq 1 ]] && exec ./bin/@NAME@
)SH";

const char* kCMakeLists = R"CM(cmake_minimum_required(VERSION 3.20)
project(@NAME@ LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# UILO is vendored under ext/ by build.sh, which also provisions UILO's own
# third-party dependencies. Configuring without it is a mistake worth catching
# here rather than fifty lines into UILO's own CMake.
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ext/UILO/CMakeLists.txt")
    message(FATAL_ERROR
        "ext/UILO is missing. Run ./build.sh -- it fetches UILO and its "
        "dependencies, then configures this project.")
endif()

add_subdirectory(ext/UILO)

# CONFIGURE_DEPENDS so a new file under src/ is picked up without re-running
# CMake by hand.
file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_executable(@NAME@ ${APP_SOURCES})

# uilo carries its own include directories, C++20 requirement and link
# libraries as PUBLIC usage requirements, so this one line is the whole of it.
target_link_libraries(@NAME@ PRIVATE uilo)

# Straight into bin/, including under multi-config generators, which otherwise
# append Debug/ or Release/ of their own.
set_target_properties(@NAME@ PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY         "${CMAKE_CURRENT_SOURCE_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG   "${CMAKE_CURRENT_SOURCE_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/bin")
)CM";


const char* kGitignore = R"GI(# Fetched by build.sh -- UILO and everything it vendors.
/ext/

# Build output.
/build/
/bin/*
!/bin/.gitkeep

.DS_Store
compile_commands.json
)GI";


const char* kReadme = R"MD(# @NAME@

A [UILO](@REPO@) application.

## Build

```bash
./build.sh            # Release
./build.sh debug      # Debug
./build.sh run        # build, then run it
./build.sh clean      # wipe build/, keep the fetched dependencies
```

The first run fetches UILO into `ext/UILO` and has it vendor SDL3 and bgfx, so
it takes a while. Later runs reuse all of it.

`build.sh` also checks for git, CMake, Python 3, a C++ compiler and, on Linux,
the X11/Wayland development packages SDL3 needs -- and offers to install
anything missing with your package manager. Pass `--yes` to skip the prompts or
`--no-install` to only be told what is missing.

On Windows, run it from Git Bash: UILO drives MSVC through CMake's Visual Studio
generator, so there is no separate `.bat` or `.ps1`.

## Layout

```
bin/     the built binary
ext/     UILO and its vendored dependencies (fetched, not committed)
src/     your code -- every .cpp under here is compiled
```

## Where to look next

`src/main.cpp` is a working window. UILO's own README and `ext/UILO/examples/`
cover the rest: layout with `_px`/`_pct`/`_flex`, palettes and themes, and the
widget set.
)MD";

const char* kMainMinimal = R"CPP(// @NAME@ -- a UILO application.
//
// The shape of every UILO program: build a page, hand it to a UILO, then loop
// polling events, updating the tree and drawing it.

#include <UILO.hpp>
#include <renderer/Renderer.hpp>

#include <cstdio>
#include <string>

using namespace uilo;

static Palette palette() {
    Palette p;
    p.set("app.bg",   { 24,  26,  35, 255});
    p.set("panel",    { 38,  41,  54, 255});
    p.set("panelAlt", { 52,  56,  73, 255});
    p.set("text",     {206, 214, 238, 255});
    p.set("textDim",  {132, 141, 172, 255});
    p.set("accent",   {126, 160, 240, 255});
    p.set("onAccent", {255, 255, 255, 255});
    p.set("outline",  { 62,  67,  84, 255});
    return p;
}

int main() {
    Renderer renderer;
    if (!renderer.init(900, 600, "@NAME@", 16)) {
        std::fprintf(stderr, "failed to initialise the renderer\n");
        return 1;
    }

    UILO ui;
    ui.setRenderer(renderer);
    ui.setPalette(palette());
    ui.setScale(OS::scale());

    int clicks = 0;

    // Held so the click handler can rewrite it. Text keeps a live string of its
    // own, so runtime updates go through setString rather than the options.
    Text* counter = text(
        Modifier()
            .setHeight(30_px),
        TextOptions()
            .setContent("No clicks yet")
            .setCharSize(18)
            .setColorRole("textDim")
            .setTextAlignX(Align::CenterX)
            .setTextAlignY(Align::CenterY)
    );

    ui.addPage(page(
        column(
            Modifier(),
            ColumnOptions()
                .setColorRole("app.bg")
                .setInnerPadding(32.f),
            contains{
                // A share of what is left, above and below, which centres the
                // card without any absolute positioning.
                spacer(Modifier().setHeight(1_flex)),

                column(
                    Modifier()
                        .setHeight(200_px),
                    ColumnOptions()
                        .setColorRole("panel")
                        .setRounding(12.f)
                        .setInnerPadding(24.f)
                        .setOutlineColorRole("outline")
                        .setOutlineThickness(1.f),
                    contains{
                        text(
                            Modifier()
                                .setHeight(40_px),
                            TextOptions()
                                .setContent("Hello, @NAME@")
                                .setCharSize(28)
                                .setColorRole("text")
                                .setTextAlignX(Align::CenterX)
                                .setTextAlignY(Align::CenterY)
                        ),
                        counter,
                        spacer(Modifier().setHeight(1_flex)),
                        button(
                            Modifier()
                                .setHeight(44_px)
                                .setWidth(200_px)
                                .setAlign(Align::CenterX)
                                .setOnLeftClick([&counter, &clicks](Element*) {
                                    ++clicks;
                                    counter->setString(clicks == 1
                                        ? "1 click"
                                        : std::to_string(clicks) + " clicks");
                                }),
                            ButtonOptions()
                                .setColorRole("accent")
                                .setRounding(8.f)
                                .setLabel(text(
                                    Modifier(),
                                    TextOptions()
                                        .setContent("Click me")
                                        .setCharSize(16)
                                        .setColorRole("onAccent")
                                        .setTextAlignX(Align::CenterX)
                                        .setTextAlignY(Align::CenterY)
                                ))
                        ),
                    }
                ),

                spacer(Modifier().setHeight(1_flex)),
            },
            "root"
        ),
        "main"
    ));
    ui.setPage("main");

    while (ui.isRunning()) {
        ui.pollEvents();
        ui.update();

        renderer.beginFrame();
        renderer.clear(ui.getPalette().get("app.bg"));
        ui.render();
        renderer.endFrame();
    }
    return 0;
}
)CPP";


const char* kMainPanels = R"CPP(// @NAME@ -- a UILO application.
//
// An application-shaped layout: a title bar, a sidebar and a content area.
// Sizes are pixels for the things that should not move and _flex for the things
// that should absorb the rest, which is the whole of UILO's layout model.

#include <UILO.hpp>
#include <renderer/Renderer.hpp>

#include <cstdio>
#include <string>

using namespace uilo;

static Palette palette() {
    Palette p;
    p.set("app.bg",   { 24,  26,  35, 255});
    p.set("panel",    { 38,  41,  54, 255});
    p.set("panelAlt", { 52,  56,  73, 255});
    p.set("text",     {206, 214, 238, 255});
    p.set("textDim",  {132, 141, 172, 255});
    p.set("accent",   {126, 160, 240, 255});
    p.set("onAccent", {255, 255, 255, 255});
    p.set("outline",  { 62,  67,  84, 255});
    return p;
}

static Text* g_status = nullptr;

static Button* navItem(const std::string& name) {
    return button(
        Modifier()
            .setHeight(38_px)
            .setOnLeftClick([name](Element*) {
                if (g_status) g_status->setString("Selected " + name);
            }),
        ButtonOptions()
            .setColorRole("panelAlt")
            .setRounding(6.f)
            .setLabel(text(
                Modifier(),
                TextOptions()
                    .setContent(name)
                    .setCharSize(15)
                    .setColorRole("text")
                    .setTextAlignX(Align::CenterX)
                    .setTextAlignY(Align::CenterY)
            ))
    );
}

int main() {
    Renderer renderer;
    if (!renderer.init(1100, 680, "@NAME@", 16)) {
        std::fprintf(stderr, "failed to initialise the renderer\n");
        return 1;
    }

    UILO ui;
    ui.setRenderer(renderer);
    ui.setPalette(palette());
    ui.setScale(OS::scale());

    g_status = text(
        Modifier()
            .setHeight(28_px),
        TextOptions()
            .setContent("Pick something on the left")
            .setCharSize(15)
            .setColorRole("textDim")
            .setTextAlignX(Align::Left)
            .setTextAlignY(Align::CenterY)
    );

    ui.addPage(page(
        column(
            Modifier(),
            ColumnOptions()
                .setColorRole("app.bg")
                .setInnerPadding(10.f),
            contains{
                // Fixed height: the title bar keeps its size whatever the window
                // does.
                row(
                    Modifier()
                        .setHeight(56_px),
                    RowOptions()
                        .setColorRole("panel")
                        .setRounding(10.f)
                        .setInnerPadding(16.f),
                    contains{
                        text(
                            Modifier()
                                .setAlign(Align::Left | Align::CenterY),
                            TextOptions()
                                .setContent("@NAME@")
                                .setCharSize(20)
                                .setColorRole("text")
                                .setTextAlignX(Align::Left)
                                .setTextAlignY(Align::CenterY)
                        ),
                    }
                ),

                spacer(Modifier().setHeight(10_px)),

                // A share of the rest: the body takes whatever the bar left.
                row(
                    Modifier()
                        .setHeight(1_flex),
                    RowOptions(),
                    contains{
                        // Fixed-width sidebar.
                        column(
                            Modifier()
                                .setWidth(220_px),
                            ColumnOptions()
                                .setColorRole("panel")
                                .setRounding(10.f)
                                .setInnerPadding(12.f),
                            contains{
                                navItem("Overview"),
                                spacer(Modifier().setHeight(8_px)),
                                navItem("Details"),
                                spacer(Modifier().setHeight(8_px)),
                                navItem("Settings"),
                                spacer(Modifier().setHeight(1_flex)),
                            }
                        ),

                        spacer(Modifier().setWidth(10_px)),

                        // And the content area takes the rest of the width.
                        column(
                            Modifier()
                                .setWidth(1_flex),
                            ColumnOptions()
                                .setColorRole("panel")
                                .setRounding(10.f)
                                .setInnerPadding(20.f),
                            contains{
                                text(
                                    Modifier()
                                        .setHeight(34_px),
                                    TextOptions()
                                        .setContent("Content")
                                        .setCharSize(22)
                                        .setColorRole("text")
                                        .setTextAlignX(Align::Left)
                                        .setTextAlignY(Align::CenterY)
                                ),
                                g_status,
                                spacer(Modifier().setHeight(1_flex)),
                            }
                        ),
                    }
                ),
            },
            "root"
        ),
        "main"
    ));
    ui.setPage("main");

    while (ui.isRunning()) {
        ui.pollEvents();
        ui.update();

        renderer.beginFrame();
        renderer.clear(ui.getPalette().get("app.bg"));
        ui.render();
        renderer.endFrame();
    }
    return 0;
}
)CPP";

/*
    create(const Options& options):
    - Params:   const Options& options
    - Returns:  Result
    - Desc:     Writes the project: the three folders the layout is built around
                -- bin for output, ext for fetched dependencies, src for code --
                plus the CMake file, the build script, a starter main.cpp, a
                README and a .gitignore. Nothing is fetched here; build.sh does
                that on its first run, so generating a project needs no network.
*/
Result create(const Options& options) {
    Result result;
    result.root = options.path;

    if (std::string bad = validate(options); !bad.empty()) {
        result.error = bad;
        return result;
    }

    const std::string name = options.name.empty()
        ? options.path.filename().string()
        : options.name;

    auto fill = [&](const char* templateText) {
        std::string text = substitute(templateText, "@NAME@", name);
        text = substitute(text, "@REPO@", options.uiloRepo);
        return substitute(text, "@REF@", options.uiloRef);
    };

    struct Entry { const char* relative; std::string content; };
    const std::vector<Entry> files = {
        { "CMakeLists.txt", fill(kCMakeLists) },
        { "build.sh",       fill(kBuildScript) },
        { ".gitignore",     fill(kGitignore) },
        { "README.md",      fill(kReadme) },
        { "src/main.cpp",   fill(options.tmpl == Template::Panels ? kMainPanels : kMainMinimal) },
        /* bin and ext are empty until build.sh runs, and an empty directory is
           not something git will carry, so each gets a keep file. */
        { "bin/.gitkeep",   "" },
        { "ext/.gitkeep",   "" },
    };

    std::error_code ec;
    fs::create_directories(options.path, ec);
    if (ec) {
        result.error = "could not create " + options.path.string() + ": " + ec.message();
        return result;
    }

    for (const Entry& file : files) {
        std::string error;
        if (!writeFile(options.path / file.relative, file.content, error)) {
            result.error = error;
            return result;
        }
        result.written.push_back(file.relative);
    }

    /* The build script has to be runnable; on Windows this is a no-op and the
       script is invoked through bash anyway. */
    fs::permissions(options.path / "build.sh",
                    fs::perms::owner_all | fs::perms::group_read | fs::perms::group_exec
                                         | fs::perms::others_read | fs::perms::others_exec,
                    fs::perm_options::replace, ec);

    result.ok = true;
    return result;
}

} // namespace uilonew
