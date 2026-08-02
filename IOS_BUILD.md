# Building a UILO app for iOS

## Read this first

**I have not built this.** This machine has only the Command Line Tools, no iOS
SDK, so nothing below was compiled or run on a device or simulator. What I did
do is go through UILO's own source and build system for everything that assumes
macOS, fix the parts that were actually wrong, and write down the rest. Treat
the CMake and Xcode specifics as a starting point that will need adjusting, and
the "what UILO does" sections as accurate — those came from the code.

What was verified:

- The three places that would have broken an iOS build are fixed (below), and
  the macOS build still compiles clean and passes the full suite afterwards.
- `MacStubs.cpp` provides every symbol the macOS shims do, so an iOS link
  resolves.

What was not verified: the CMake invocation, the Xcode project settings, the run
loop, touch behaviour, and anything about signing or the simulator.

---

## What was wrong, and is now fixed

`__APPLE__` is defined on iOS as well as macOS. Three places leaned on it to
mean "macOS", which made an iOS build impossible:

| File | Was | Now |
|---|---|---|
| `include/platform/MacScroll.mm` | `#ifdef __APPLE__` around AppKit code | `#if defined(__APPLE__) && TARGET_OS_OSX` |
| `include/platform/MacWindow.mm` | same | same |
| `include/platform/MacStubs.cpp` | compiled only when `!__APPLE__` | compiled whenever not macOS, so iOS gets it |
| `CMakeLists.txt` | `elseif(APPLE)` added the `.mm` files | also requires `NOT CMAKE_SYSTEM_NAME STREQUAL "iOS"` |
| `include/platform/Pty.cpp` | POSIX `forkpty` branch on all Apple | iOS takes the "unsupported" branch |

Without these, an iOS build would have tried to compile AppKit against the iOS
SDK and failed outright.

---

## What works on iOS, and what does not

**Works, or should:**

- **Rendering.** bgfx uses Metal on Apple platforms, and `Renderer` already
  hands it a `CAMetalLayer` via `SDL_Metal_CreateView` / `SDL_Metal_GetLayer`
  (`Renderer.cpp`), which is the same path iOS needs. The Metal shader profile
  is already built, because the shader block keys off `if(APPLE)`.
- **Fonts and icons.** Both typefaces and the whole icon set are compiled into
  the binary, so nothing has to be found on disk at runtime. This removes most
  of the usual iOS asset-bundling pain — see *Assets* below.
- **Display scale.** `OS::scale()` falls back to `SDL_GetDisplayContentScale`
  when it cannot query a physical panel size, which is the right answer on iOS.

**Does not work:**

- **`Terminal`.** It needs `forkpty`, and a sandboxed iOS app may not spawn
  processes. `Pty::open` now fails with *"iOS does not allow an app to spawn a
  shell"* and the widget renders an empty screen reporting that, rather than
  crashing. Do not ship a terminal on iOS.
- **The macOS trackpad and live-resize shims.** `installMacScrollMonitor`,
  `configureMacWindowForLiveResize` and friends become no-ops from
  `MacStubs.cpp`. You lose UILO's synthesised scroll momentum; scrolling falls
  back to whatever SDL reports. There is no window to live-resize anyway.
- **Hover.** There is no pointer, so `onHoverEnter` / `onHoverExit` and
  `Modifier::setCursor` are meaningless. Anything whose affordance is
  hover-only needs a different design on touch.

---

## Prerequisites

- **Full Xcode**, not just the Command Line Tools. Check with
  `xcrun --sdk iphoneos --show-sdk-path`; if that errors, you have only the CLT.
- An Apple Developer account if you want to run on a physical device. The
  simulator does not need one.
- CMake 3.14 or newer.
- A macOS build of UILO completed at least once (`./build.sh`). You need the
  host tools it produces — see the next section.

---

## Step 1 — keep a host `shaderc`

UILO compiles its shaders at build time with bgfx's `shaderc`, imported here:

```cmake
set(UILO_BGFX_BIN_DIR "${UILO_EXT_DIR}/bgfx/.build/${UILO_BGFX_PLATFORM_DIR}/bin")
```

`shaderc` is a **host** tool — it has to run on your Mac, not on the phone. A
normal `./build.sh` puts a macOS one at `ext/bgfx/.build/osx-arm64/bin`. Leave
it there.

You also need bgfx, bx and bimg built **for iOS**, because those get linked into
the app. bgfx's own GENie build supports it:

```bash
cd ext/bgfx
../bx/tools/bin/darwin/genie --gcc=ios-arm64 --with-tools gmake
make -C .build/projects/gmake-ios-arm64 config=release
```

That produces `ext/bgfx/.build/ios-arm64/bin`. UILO's CMake does not know about
that directory yet — it only maps `osx-arm64`, `osx-x64`, `win64_vs2022` and
`linux64_gcc`. Add a branch:

```cmake
if(APPLE)
    if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
        set(UILO_BGFX_PLATFORM_DIR "ios-arm64")     # or ios-simulator-arm64
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
        set(UILO_BGFX_PLATFORM_DIR "osx-arm64")
    else()
        set(UILO_BGFX_PLATFORM_DIR "osx-x64")
    endif()
```

and keep `bgfx::shaderc` pointing at the **host** bin directory, not this one:

```cmake
set_target_properties(bgfx::shaderc PROPERTIES
    IMPORTED_LOCATION "${UILO_EXT_DIR}/bgfx/.build/osx-arm64/bin/shadercRelease")
```

This split — host tool, target library — is the single most likely thing to go
wrong. If the build complains that `shadercRelease` cannot execute, it is
because a cross-compiled one ended up on the path.

## Step 2 — configure

iOS needs the Xcode generator; the Makefile and Ninja generators cannot produce
a signable app bundle.

```bash
cmake -S . -B build/ios -G Xcode \
      -DCMAKE_SYSTEM_NAME=iOS \
      -DCMAKE_OSX_DEPLOYMENT_TARGET=13.0 \
      -DCMAKE_OSX_ARCHITECTURES=arm64 \
      -DUILO_BUILD_EXAMPLES=OFF \
      -DCMAKE_XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH=NO
```

For the simulator, add `-DCMAKE_OSX_SYSROOT=iphonesimulator` and point
`UILO_BGFX_PLATFORM_DIR` at a simulator build of bgfx.

Turn the examples off. Every one of them is a desktop program with a `main()`
and a `while` loop, and `examples/terminal.cpp` cannot work on iOS at all.

## Step 3 — the app target

An iOS executable has to be a bundle:

```cmake
add_executable(MyApp MACOSX_BUNDLE main.cpp)
target_link_libraries(MyApp PRIVATE uilo)

set_target_properties(MyApp PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER        com.example.myapp
    MACOSX_BUNDLE_BUNDLE_NAME           MyApp
    MACOSX_BUNDLE_SHORT_VERSION_STRING  1.0
    XCODE_ATTRIBUTE_DEVELOPMENT_TEAM    "YOURTEAMID"
    XCODE_ATTRIBUTE_CODE_SIGN_STYLE     Automatic)
```

SDL needs several system frameworks. If SDL3's own CMake is pulling them in for
you this is already handled; if you are linking a prebuilt SDL, you will need at
least `UIKit`, `Metal`, `QuartzCore`, `CoreGraphics`, `Foundation`,
`AVFoundation`, `AudioToolbox`, `CoreMotion`, `GameController` and
`CoreHaptics`.

You will also want a launch storyboard. Without one iOS letterboxes the app at a
legacy size and the window comes out the wrong resolution — which will look like
a UILO scaling bug and is not.

## Step 4 — `main()` and the run loop

The desktop shape every UILO example uses:

```cpp
while (uilo.isRunning()) {
    uilo.pollEvents();
    uilo.update();
    renderer.beginFrame();
    renderer.clear();
    uilo.render();
    renderer.endFrame();
}
```

is the wrong shape for iOS. The system expects to own the loop and to be able to
suspend you; a `while` loop that never returns fights that. Use SDL3's main
callbacks instead, keeping the *body* of the loop intact:

```cpp
#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL_main.h>
#include <UILO.hpp>

using namespace uilo;

namespace { Renderer* g_renderer = nullptr; UILO* g_uilo = nullptr; }

SDL_AppResult SDL_AppInit(void**, int, char**) {
    g_renderer = new Renderer();
    g_renderer->init(0, 0, "MyApp");        // full screen on iOS
    g_uilo = new UILO(*g_renderer, buildMainPage());
    g_uilo->setScale(OS::scale());
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void*, SDL_Event* e) {
    return e->type == SDL_EVENT_QUIT ? SDL_APP_SUCCESS : SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void*) {
    g_uilo->pollEvents();
    g_uilo->update();
    g_renderer->beginFrame();
    g_renderer->clear();
    g_uilo->render();
    g_renderer->endFrame();
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void*, SDL_AppResult) { delete g_uilo; delete g_renderer; }
```

One UILO detail: `UILO::update()` installs the macOS scroll and zoom monitors on
its first call. On iOS those resolve to the stubs in `MacStubs.cpp` and return
`false`, so this costs one wasted call and nothing else.

## Step 5 — input

UILO handles mouse events only; there is no touch handling anywhere in
`UILO.cpp`. It relies on SDL turning touches into synthetic mouse events, which
SDL does by default. A tap therefore reaches `checkLeftClick` and buttons work.

What does not survive the translation:

- **Hover.** A finger has no hover state. Elements that only reveal themselves
  on `onHoverEnter` will never do so. Design for tap.
- **Momentum scrolling.** UILO synthesises its own from the macOS trackpad
  monitor, which is a stub here, so scrolling is whatever raw deltas SDL
  produces from a drag — expect it to feel worse than a native list.
- **Right click.** No equivalent; `onRightClick` will not fire.
- **Hit target size.** Anything sized for a mouse pointer will be too small.
  Apple's guidance is 44×44 points minimum.

If touch scrolling feels wrong, the place to look is `Terminal::checkScroll` and
`Column::checkScroll`, both of which scale a delta by `scrollSpeed` and treat
`precise` as "a trackpad sent pixels". A touch drag is closer to the precise
case than the wheel case.

## Step 6 — scale and the safe area

Call `uilo.setScale(OS::scale())` as on desktop. On iOS this comes from
`SDL_GetDisplayContentScale`, so it will be 2 or 3 and the layout will be in
points, which is what you want.

UILO has no concept of a safe area, so content will run under the notch, the
Dynamic Island and the home indicator. Until it does, inset your root container
yourself. `SDL_GetWindowSafeArea` gives you the rectangle; feed the insets into
the root's `setOuterPadding` or a set of `Spacer`s.

## Step 7 — assets

Most of this problem is already gone: both fonts and every icon are compiled
into the binary, so the common cases need no files at all.

```cpp
text({}, TextOptions().setContent("hi"));                       // built-in DejaVu Sans
text({}, TextOptions().setFont(Resources::fonts::mono));        // built-in Droid Sans Mono
icon({}, IconOptions().setIcon(Resources::icons::settings));    // built-in
```

For anything else — images, your own fonts — the working directory on iOS is not
where the desktop examples assume. A relative path like
`"assets/fonts/Montserrat.ttf"` will not resolve. Add the files to the bundle:

```cmake
target_sources(MyApp PRIVATE assets/logo.png)
set_source_files_properties(assets/logo.png PROPERTIES
    MACOSX_PACKAGE_LOCATION Resources)
```

and build the path at runtime from `SDL_GetBasePath()`, which returns the
bundle's resource directory on iOS.

## Step 8 — run it

```bash
cmake --build build/ios --config Release
```

then open `build/ios/UILO.xcodeproj` and run from Xcode, which handles the
install and the signing. Command-line installs to a device are possible with
`xcrun devicectl` but Xcode is the shorter path the first time.

---

## Checklist when it does not work

| Symptom | Most likely cause |
|---|---|
| AppKit headers not found | The guard fixes above are missing or reverted |
| `shadercRelease` cannot execute | `shaderc` was cross-compiled; point it at the host build |
| `prebuilt bgfx not found` | No `UILO_BGFX_PLATFORM_DIR` branch for iOS |
| Links but shows a black screen | Metal layer never created — check `Renderer::init` got a real window |
| Everything is half size | `setScale(OS::scale())` not called, or no launch storyboard |
| Text renders but images do not | Relative asset paths; use `SDL_GetBasePath()` |
| Terminal shows an error line | Expected. iOS forbids spawning a shell |

## What UILO would need for iOS to be properly supported

Honest list, none of it done:

1. **Touch as a first-class input**, rather than relying on synthetic mouse
   events — real drag/fling scrolling with momentum, and a tap target model.
2. **Safe-area insets** plumbed into layout, so a root container can avoid the
   notch without the app doing arithmetic.
3. **An iOS branch in `CMakeLists.txt`** for the bgfx platform directory and the
   host/target `shaderc` split, so none of Step 1 is manual.
4. **A lifecycle story** — `SDL_EVENT_WILL_ENTER_BACKGROUND` and friends. bgfx
   needs to stop rendering when the app is backgrounded or iOS will kill it.
5. **An example** that is actually a touch app, since every current example is a
   desktop program with a `while` loop.
