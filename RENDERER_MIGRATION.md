# Migration: bgfx + SDL → VALO

Checklist for moving UILO onto VALO ([ext/VALO](ext/VALO)). VALO replaces
both bgfx (rendering) **and** SDL (window, input, platform). When this is done,
UILO has no direct dependency on either, and nothing from SDL appears in UILO's
public API. VALO's own public API is fixed. Anything it's missing is listed in §2
as an *addition* to VALO, with a UILO-side fallback where one exists.

UILO's public API stays the same apart from two intended changes:
1. **The material/glass system is removed (§0).**
2. **SDL types are removed from the public API (§1).**

The `Renderer` swap is done at link time, the same way
[include/wt/HeadlessBackend.cpp](include/wt/HeadlessBackend.cpp) already
provides a second implementation for the Wt build.

VALO is software-only for now, so everything here targets the software path.
§12 lists what to re-check once VALO's Vulkan backend exists.

---

## How bgfx/SDL concepts map onto VALO

| bgfx / SDL (today) | VALO |
|---|---|
| `SDL_CreateWindow` + `SDL_Init`/`SDL_Quit` | `valo::Window(title, size, WinFlags::Resize \| WinFlags::HiDPI)`, which does its own init/quit |
| `SDL_PollEvent` loop → `UILO::handleEvent` | `window.pollEvents()`, then read per-frame state (`wasKeyPressed`, `wasMousePressed`, `scrollDelta`, `mouseDelta`, …) |
| `SDL_GetKeyboardState` / `SDL_GetMouseState` | `window.isKeyDown(Key)` / `isMouseDown(MouseButton)` / `mousePosition()` (already in pixels) |
| `SDL_Scancode` | `valo::Key` (its values are SDL scancodes) |
| `SDL_BUTTON_*` | `valo::MouseButton` |
| `SDL_SetWindowRelativeMouseMode` | `window.grabMouse()` / `releaseMouse()` / `isMouseGrabbed()` |
| `SDL_GetWindowSizeInPixels` | `window.getSize()` |
| `SDL_SetWindowTitle` | `window.setTitle()` |
| `SDL_GetTicks` | `valo::Clock` (or `std::chrono`) |
| `bgfx::init` on the native window | `VALO::init(window, cfg)` |
| `bgfx::frame()` | `VALO::endFrame()` (executes every queued command, then presents) |
| `bgfx::reset` on resize | Automatic: `VALO::beginFrame` checks `window.resized()` |
| Scene view + offscreen FB + composite | `BASE_LAYER` drawing straight into `BACKBUFFER` |
| Framebuffer / render target | `createTarget(w, h)`. Targets and textures share one ID space, so any target can be sampled with `texture(id, uv)` |
| Transient VB/IB | None. Use one persistent **arena** VB/IB, uploaded once per frame (§5) |
| `setState` blend | `RenderCommand::blendmode` = `Opaque` / `Alpha` / `Additive` |
| `setScissor` | None yet. Use `pushClip` with a rect quad |
| Rounded-clip uniforms in every shader | `pushClip(shapeCmd)` / `popClip`, a stencil stack that nests to any depth |
| `setUniform` vec4s | `RenderCommand::setUniforms(struct)`, up to 512 B per command |
| `.sc` shaders compiled by shaderc | C++ functions `vs(const V&, const U&, Vy&)` / `fs(const Vy&, const U&)` writing `valo_Position` / `valo_FragColor` |
| `bgfx::getStats()` | None. Count commands in UILO and time `endFrame` |
| 16-bit indices | 32-bit indices, so the 65535-vertex flush logic goes away |

---

## 0. Remove the material system first

Do this on bgfx, before starting the port. That way the port never has to deal
with the offscreen scene target, blur ladder, deferred glass or glass views.

- [ ] Delete [include/utils/Material.hpp](include/utils/Material.hpp)
- [ ] `Renderer.hpp`: remove `drawGlass`, `setMouseState`, `beginGlassSubtree`, `endGlassSubtree`, the `Material.hpp` include, and the `m_mousePos*` / `m_mouseLastMoveT` members
- [ ] `Renderer_Texture.cpp`: remove `drawGlass`
- [ ] `Renderer.cpp` / `RendererImpl.hpp`: remove the glass and blur programs and uniforms, `sceneFB`/`blurFB_A`/`blurFB_B`, `ensureSceneFramebuffers`, `runBlurPasses`, `compositeSceneToBackbuffer`, `deferredGlass`, the bypass flags and the glass view IDs. The scene then renders straight to the backbuffer.
- [ ] Delete the shaders `fs_glass.sc` and `fs_blur.sc`, and drop them from the CMake shader list and the embedded-shader table
- [ ] `Modifier.hpp/.cpp`: remove `setMaterial` / `getMaterial`
- [ ] `Row.cpp/.hpp`, `Column.cpp/.hpp`: remove the material background path and the glass-subtree calls
- [ ] `Dropdown.cpp`, `UILO.cpp` (the `setMouseState` call), `utils/Gradient.hpp`, `wt/Translator.cpp`: remove their Material references
- [ ] `HeadlessBackend.cpp`: remove the four stubs
- [ ] Examples: delete `glass_trail_test.cpp` and strip materials from `containers.cpp`
- [ ] Docs: delete `docs/utils/Material.hpp.md`; update the `docs/README.md` index and the Row, Column, Modifier, Gradient and renderer pages
- [ ] Build and run every example on bgfx to confirm nothing regressed before moving on

---

## 1. Remove SDL from UILO's public API

**Decisions first:**
- [ ] **Key and button types in UILO's API:** use `valo::Key` / `valo::MouseButton` directly, or re-export them as `uilo::Key` / `uilo::MouseButton` (`using Key = valo::Key;`). The re-export keeps VALO's names out of user code and costs nothing.
- [ ] **Event model:** UILO is event-driven today (`handleEvent(const SDL_Event&)`), and `valo::Window` is polled state. Recommended: drop `handleEvent`, make `UILO::pollEvents()` call `window.pollEvents()` and read the state, and have every example use `ui.pollEvents()`. The alternative is a UILO-owned event struct, which needs VALO to expose an event queue (§2b).
- [ ] **Window access:** `Renderer::sdlWindow()` goes away. Either expose the `valo::Window&`, or keep it private and give UILO's .cpp files an internal accessor. Nothing outside UILO calls it except `tools/ctxmenu_probe`.

**Public headers to change:**
- [ ] [UILO.hpp](include/UILO.hpp): remove `handleEvent(const SDL_Event&)` (or replace it per the decision above); update the `pollEvents` / `setRenderer` / `queryMousePixelPosition` comments
- [ ] [Renderer.hpp](include/renderer/Renderer.hpp): remove `struct SDL_Window;`, `sdlWindow()`, `m_window`, and `attach(SDL_Window*, uint16_t)`. `attach` has no callers, and VALO can't share a host's window or composite transparently (§2a), so remove it or replace it later.
- [ ] [input/Keybinds.hpp](include/input/Keybinds.hpp): drop `<SDL3/SDL.h>`; `SDL_Scancode` → `Key`; `SDL_GetKeyboardState` → `window.isKeyDown`. It's header-only, so it needs the window passed in, or its implementation moved to a .cpp.
- [ ] [input/Mousebinds.hpp](include/input/Mousebinds.hpp): drop `<SDL3/SDL.h>`; `SDL_Window*` → `valo::Window*`; button masks → `MouseButton`; relative mode → `grabMouse`/`releaseMouse`; relative delta → `mouseDelta()`. That delta is already multiplied by pixel density, so remove the extra `SDL_GetWindowDisplayScale` multiply.
- [ ] [Interactible.hpp](include/elements/interactible/Interactible.hpp): drop `<SDL3/SDL.h>`; change `handleKeyInput(SDL_Keycode, …)` to UILO's key type (see §2b on keycodes); update the `SDL_StartTextInput` comment
- [ ] [Textbox.hpp](include/elements/interactible/Textbox.hpp), [Terminal.hpp](include/elements/widgets/Terminal.hpp): `handleKeyInput` and `sendKey(SDL_Keycode, …)` signatures
- [ ] [Knob.hpp](include/elements/interactible/Knob.hpp), [Slider.hpp](include/elements/interactible/Slider.hpp): drop the unused-in-header `<SDL3/SDL.h>` include

**Implementation files that use SDL directly** (port each onto `valo::Window`, or onto a §2b addition):
- [ ] `UILO.cpp`: events, text input, modifiers, live-resize watch, the Cocoa handle for the macOS hooks
- [ ] `Textbox.cpp`, `Terminal.cpp`: `SDLK_*` handling, modifiers, clipboard
- [ ] `Canvas.cpp`: `SDL_GetModState`
- [ ] `Knob.cpp`, `Slider.cpp`, `Resizer.cpp`: `SDL_GetTicks` → `valo::Clock` / `std::chrono`
- [ ] `Container.cpp`: `SDL_GetMouseState` + left-button mask during a drag → `window.isMouseDown(MouseButton::Left)` / `mousePosition()`
- [ ] `utils/OS.cpp`: displays, theme, CPU, RAM, paths (see §2b)
- [ ] `renderer/*`: goes away with the port
- [ ] **Examples** that run their own `SDL_PollEvent` loop (`gradients`, `pixel_test`, `render_bench`): switch to `ui.pollEvents()`. Also remove the SDL includes from `containers`, `contextmenu`, `datepicker`, `terminal`, `theme`.
- [ ] `tools/ctxmenu_probe.cpp`: it synthesises input with `SDL_PushEvent`, which VALO has no equivalent for. Rework it or retire it.
- [ ] **Wt build:** `wt/shim/SDL3/SDL.h` and the SDL stubs in `HeadlessBackend.cpp` exist only because UILO called SDL. Replace them with stubs for whatever `valo::Window` surface UILO ends up using, or keep VALO out of the Wt build behind the same `UILO_WT` guard.
- [ ] Verify with `grep -rn "SDL" include/*.hpp include/**/*.hpp` returning nothing except comments you mean to keep

---

## 2. VALO gaps

None of these change VALO's existing API; they are additions.

### 2a. Rendering (all already on [VALO's TODO](ext/VALO/TODO.md))

- [ ] **`updateTexture(id, data)`**. UILO needs it for glyph atlases and `Renderer::updateTexture` (Icon, Image).
  - Workaround: `destroyTexture` + `createTexture` once per frame for each dirty texture, done in UILO's `endFrame` *before* `valo.endFrame`. VALO's free list is LIFO, so the ID normally comes back the same, but check it and remap if it doesn't.
- [ ] **R8 texture format**. Workaround: store glyph atlases as RGBA8 with coverage in alpha. That's 4 MB per 1024² atlas instead of 1 MB.
- [ ] **`fwidth` / derivatives**. Workaround: compute AA analytically. Every UI shader works in pixel space, so an SDF distance is already in pixels and a ~1px smoothstep is correct.
- [ ] **Scissor on `RenderCommand`**. Workaround: a rect `pushClip` (§7), which costs two extra raster passes over the rect.
- [ ] **Mipmaps**. Without them, heavily downscaled images alias. It's acceptable for now.

Things VALO does differently that UILO has to live with:
- [ ] **Alpha blend forces destination alpha to 1**, so a transparent overlay over a host app's image is impossible (hence removing `attach`).
- [ ] **Stencil clips have hard edges.** Children spilling past a rounded container's corner lose their AA. The container's own background is still an AA'd SDF. Accept it, or supersample with `renderScale` > 1 (slow in software).

### 2b. Window / input / platform

`valo::Window::pollEvents()` drains the SDL queue itself and keeps only
per-frame bitsets, so UILO can't read these events on its own. The only
UILO-side fallback for event-type gaps is an `SDL_AddEventWatch` installed
inside a UILO .cpp through `window.sdlHandle()`. That keeps SDL out of the
public API but still links UILO to SDL internally. Adding them to VALO is the
clean fix.

| # | Needed by UILO | Used by | `valo::Window` today | Fix |
|---|---|---|---|---|
| 1 | **Text input** (UTF-8 text events) + start/stop text input | Textbox, Terminal, `UILO.cpp` | missing | [ ] add to VALO: e.g. `textInput()` returning this frame's UTF-8, plus `startTextInput()`/`stopTextInput()` |
| 2 | **Key repeat** | holding Backspace/arrows in Textbox/Terminal | `pollEvents` ignores `e.key.repeat` | [ ] add to VALO: e.g. `wasKeyRepeated(Key)`, or count repeats in `wasKeyPressed` |
| 3 | **Layout-aware keys** (`SDLK_*`, `SDL_SCANCODE_TO_KEYCODE`) | `handleKeyInput` in Textbox (16 uses), Terminal (22), `UILO.cpp` (18) | scancodes only | [ ] decide: scancodes are fine for arrows, editing keys and F-keys, but Ctrl+Z/C/V/A on AZERTY/Dvorak need the keycode. Add a key→keycode/char query to VALO, or accept physical-position shortcuts. |
| 4 | **Ordered key events with modifiers** | `handleKeyInput(key, shift, ctrl, gui)` per keypress; the key-up vs text-input timestamp check in `UILO.cpp` | per-frame bitsets, no order, no timestamps | [ ] polled bitsets lose ordering when two keys go down in one frame. Either add an event queue to VALO, or accept frame-granular input and drop the timestamp logic. |
| 5 | Modifier state | `SDL_GetModState` in `UILO.cpp`, Canvas, Textbox | derivable | [ ] map `isKeyDown(LCtrl \|\| RCtrl)` etc. in UILO. No VALO change needed. |
| 6 | **Horizontal + precise wheel** | `dispatchScroll(pos, {dx, dy}, precise)` | `scrollDelta()` is y only | [ ] add to VALO: an x component (e.g. `scrollDelta2D()`) |
| 7 | Press position | `m_pendingPressPos` from the event's x/y | only the current position | [ ] use `mousePosition()` at poll time (fine unless the mouse moves a lot within a frame) |
| 8 | **Clipboard** get/set | Textbox copy/cut/paste, Terminal | missing | [ ] add to VALO: `getClipboardText()` / `setClipboardText()` |
| 9 | **System cursors** | `Renderer::setCursor` (Arrow, Hand, SizeH/V, Text, Crosshair) | missing | [ ] add to VALO: `setCursor(CursorType)` |
| 10 | **Live-resize redraw** | `SDL_AddEventWatch` in `UILO.cpp` so the UI keeps drawing inside the OS modal resize loop | missing | [ ] add to VALO: a resize callback (e.g. `setResizeCallback(fn)`) |
| 11 | **Native window handle** | macOS: `configureMacWindowForLiveResize`, trackpad scroll and zoom monitors (`platform/Mac*.mm`) need the `NSWindow` | `sdlHandle()` only | [ ] add to VALO: `nativeHandle()`, or move UILO's Mac hooks into VALO |
| 12 | **Display info**: primary display, content scale, bounds, refresh rate, display list | `utils/OS.cpp` | `getPixelDensity()` only | [ ] add to VALO (e.g. a `Display` query), or move `OS.cpp` onto native APIs |
| 13 | **System theme** (light/dark) | `utils/OS.cpp` | missing | [ ] add to VALO, or use native APIs |
| 14 | Base path / pref path | `utils/OS.cpp`, resources | missing | [ ] native APIs (`GetModuleFileName` / `NSBundle` / `/proc/self/exe`; `%APPDATA%` / `~/Library/Application Support` / `$XDG_DATA_HOME`), or add to VALO |
| 15 | CPU cores / system RAM | `utils/OS.cpp` | missing | [ ] `std::thread::hardware_concurrency()`; RAM via native APIs, or add to VALO |
| 16 | Quit / close | `SDL_EVENT_QUIT` → `m_running = false` | `pollEvents()` returns false, `isOpen()` | [ ] map directly; no VALO change |
| 17 | Relative mouse mode + delta | Mousebinds | `grabMouse` + `mouseDelta` | [ ] map directly (mind the density scaling, §1) |
| 18 | Timing | `SDL_GetTicks` | `valo::Clock` | [ ] map directly |
| 19 | Synthetic input | `tools/ctxmenu_probe` (`SDL_PushEvent`) | missing | [ ] rework or retire the tool (§1) |

---

## 3. Integration / build

- [ ] **UILO stops vendoring SDL3.** Remove SDL from `build.sh`, `CMakeLists.txt` and the `uilo-new` scaffold. VALO's SDL3 is the only one in the process, and nothing in UILO includes `<SDL3/SDL.h>` (apart from any §2b fallback you choose).
- [ ] **VALO's CMake uses `${CMAKE_SOURCE_DIR}`** for `ext/SDL3-install`, `ext/stb`, `ext/cgltf`, `ext/tinyobjloader` and `lib/`. Under `add_subdirectory(ext/VALO)` those resolve to UILO's root. Either fix that in VALO (`CMAKE_CURRENT_SOURCE_DIR`; this is build only, not API) or add the sources to UILO's target by hand.
- [ ] **Duplicate stb_image.** VALO's `RenderTarget.cpp` defines `STB_IMAGE_IMPLEMENTATION`, and so does UILO's `Renderer_Texture.cpp`. Keep one definition and one `stb_image.h`.
- [ ] VALO also pulls in tinyobjloader and cgltf. They're unused by UILO, but they still have to be present to build.
- [ ] **MSVC.** VALO is developed on macOS. Check that it builds with MSVC, and drop UILO's forced static runtime (`/MT`, CMakeLists ~line 49) once bgfx is gone.
- [ ] **VALO headers in UILO's public headers:** the only acceptable ones are the `Key`/`MouseButton` (and possibly `Window`) types from §1. Pull those from `render/Window.hpp` alone, never `VALO.hpp`/`Renderer.hpp`, because:
  - `Sampling.hpp` does `#define discard valo::vDiscard()`, which clobbers any identifier named `discard` in any file that includes it. Keep it in `include/renderer/*.cpp` only.
  - `valo::Color` (float `Vec4f`), `valo::Vec2f` and friends clash with `uilo::Color` / `uilo::Vec2f`. Don't `using namespace valo`.
- [ ] **Window ownership:** the `Renderer` owns a `valo::Window` built with `WinFlags::Resize | WinFlags::HiDPI` (matching today's `RESIZABLE | HIGH_PIXEL_DENSITY`), then calls `VALO::init(window, cfg)`.
  - Decide on `respectOSScale`: `false` (the default) means sizes are in pixels, which matches how UILO works in pixels today.
  - Because `window.pollEvents()` now runs every frame, `window.resized()` works and `VALO::beginFrame` resizes the targets itself.
- [ ] Shutdown order: `valo.shutdown()` first, then let the `valo::Window` destruct (it destroys the window and `SDL_Quit`s when it's the last one).
- [ ] **Remove the bgfx leftovers from `Renderer.hpp`** (private): `namespace bgfx { … }`, `FrameBuffer::viewId`, `m_nextViewId`, `m_viewStack*`, `currentViewId()`, `submitOrtho()`, `m_resetFlags`. Also delete the matching `HeadlessBackend.cpp` stubs, and fix the bgfx wording in the comments on `Texture`, `RendererStats` and `FrameBuffer`.
- [ ] [examples/containers.cpp](examples/containers.cpp): drop `#include <bgfx/bgfx.h>` and the `bgfx::getRendererName` print.
- [ ] Shaders run on VALO's worker threads, so they must not touch shared mutable state. Pass everything they need in uniforms or vertices.

---

## 4. Frame and input loop

The frame loop:

```
ui.pollEvents()      → window.pollEvents(); translate state into UILO input; quit if it returned false
ui.update()          → layout, hover, clicks, key dispatch
renderer.beginFrame()→ valo.beginFrame()  (handles resize, clears backbuffer + stencil)
renderer.clear(c)    → see §5
ui.render()          → shapes into the arena, clips via pushClip/popClip
renderer.endFrame()  → flush, upload arena, valo.endFrame(), frame limiter
```

- [ ] `UILO::pollEvents()`:
  - Mouse: position, `wasMousePressed(Left/Right)` → pending press, scroll → `dispatchScroll` (with the shift/ctrl modifiers as today)
  - Keys: `wasKeyPressed` for each key UILO handles, plus repeats (§2b #2)
  - Text input (§2b #1) → focused Interactible
  - Context-menu navigation keys
- [ ] Focus changes start and stop text input (§2b #1), as `UILO.cpp` does today with `SDL_StartTextInput`
- [ ] Keybinds / Mousebinds read from the same `valo::Window`

---

## 5. Frame structure (rendering)

With materials gone, UILO needs a single layer: VALO's default `BASE_LAYER`, which draws into `BACKBUFFER`.

**Resources created in `init`:**
- [ ] `valo::Window`, then `VALO::init(window, cfg)` with `cfg.vsync` set
- [ ] The arena VB/IB. `createVertexBuffer` needs initial data, so create it with one dummy vertex.
- [ ] The programs (§6)

**`beginFrame`:**
- [ ] `valo.beginFrame()`: resizes if the window changed, and clears colour, depth and stencil to `BASE_LAYER`'s clear colour
- [ ] Reset the arena, stats counter and rotation

**Drawing, using the arena:**
- [ ] Every shape appends its vertices and indices to CPU-side arena vectors and then extends or creates a pending command covering that index range (`firstIndex` / `indexCount`).
- [ ] **Commands reference the VB/IB by ID and only run at `valo.endFrame`**, so upload the arena exactly once per frame with `updateVertexBuffer` / `updateIndexBuffer`, just before `valo.endFrame`. Never re-upload mid-frame: earlier commands would draw the new contents.
- [ ] Command template for every UI draw:
  - `topology = Triangles`, `cullmode = None` (y-down ortho flips the winding)
  - `depthTest = false`, `depthWrite = false`, `blendmode = Alpha`
  - Always set `program`. `DEFAULT_PROGRAM` crashes (VALO TODO robustness).
- [ ] **Batching matters a lot.** Every VALO draw dispatches the thread pool over the whole target height, so a thousand tiny commands cost far more than a single one. Merge consecutive draws that share program, texture and uniforms into one command (§6).
- [ ] **Flush the pending batch before every `pushClip`/`popClip`.** `pushClip` enqueues immediately, and `submit` stamps the stencil ref at submit time.

**`endFrame`:**
- [ ] Flush the pending batch
- [ ] Re-upload any dirty textures (§2a workaround)
- [ ] Upload the arena
- [ ] `valo.endFrame()`
- [ ] Frame-rate limiter: keep UILO's sleep+spin (VALO's `RenderConfig::maxFps` is unused)

**`clear(color)`:**
- [ ] VALO clears in `beginFrame`, before UILO's `clear()` is called. Setting the layer clear colour alone would lag a frame, so:
  - `setLayerClearColor(BASE_LAYER, c)` so later frames get it for free
  - **and**, if `c` differs from what `beginFrame` just cleared to, submit a fullscreen `Opaque` quad now

---

## 6. Shaders (C++ functions)

The sources to port are `vs_solid`, `vs_tex`, `fs_solid`, `fs_tex` and `fs_text` in [include/renderer/shaders/](include/renderer/shaders/).

**Recommended: one vertex format and one "ui" program for everything.**

```cpp
struct UiVertex {                 // VtxLayout({Vec2f, Vec2f, Uint32, Vec4f, Vec4f})
    Vec2f    pos;                 // pixels
    Vec2f    uv;
    uint32_t abgr;                // uilo::packColor; unpack in vs
    Vec4f    shape;               // SDF rect: cx, cy, halfW, halfH (pixels)
    Vec4f    params;              // x = radius, y = mode (solid/rrect/tex/text/ellipse), zw spare
};
struct UiUniforms { Vec2f viewport; TextureID tex; };   // tiny, so batches merge
```

- [ ] **ui vs:** pixel → NDC from `viewport` (same convention as `clipstack.cpp`: y down, origin top-left), unpack colour, pass the varyings through
- [ ] **ui fs, solid:** vertex colour (line and arc AA comes from vertex-alpha skirts, unchanged)
- [ ] **ui fs, rounded rect / circle:** rounded-box SDF, `alpha *= 1 - smoothstep(-0.5, 0.5, d)`, `discard` at alpha 0
- [ ] **ui fs, texture:** `texture(tex, uv, Sampler(Clamp, Clamp, Linear))` × tint
- [ ] **ui fs, text:** atlas alpha × vertex colour
- [ ] **ui fs, ellipse:** texture + analytic ellipse mask
- [ ] **clip program:** the rounded-box SDF with `discard` outside, for `pushClip` shapes. It can be the ui program; VALO turns `colorWrite` off itself.
- [ ] Write the shaders in a GLSL-portable style. VALO's Vulkan backend will need a GLSL/SPIR-V twin of every program (VALO TODO §3), and with this design that's only one or two.

---

## 7. Clipping on VALO's stencil stack

VALO's `pushClip` draws the shape into the stencil with `Incr`, tested against the current depth, so nested clips intersect exactly. That replaces UILO's scissor stack, round-clip stack, clip uniforms, the dedup cache and the "only the two innermost rounded clips" limit.

- [ ] `pushRoundClip(b, r)`: flush, then `valo.pushClip(roundedRectCmd)`. Snap `b` to pixels (floor/ceil) and clamp `r` to min(halfW, halfH).
- [ ] `pushScissor(b)`: same, with radius 0
- [ ] `popRoundClip` / `popScissor`: flush, then `valo.popClip()`
- [ ] **pushRoundClip = one pushClip and popRoundClip = one popClip** (today it also pushes a scissor). Keep the push/pop counts balanced.
- [ ] Depth: the stencil is 8-bit. Keep UILO's cap of 64 and the overflow counting.
- [ ] Zero-area clip: skip draws while it's active (today's `scissorEmpty`)
- [ ] **Stop using clips for a shape's own mask.** `draw(RoundedRect)` and `draw(Circle)` use SDF mode instead.
- [ ] If framebuffers are ever implemented: `submit` stamps the stencil test on every command whatever its layer, so only switch layers when the clip stack is empty
- [ ] Behaviour change to check visually: every ancestor clip now applies, not just the two innermost rounded ones

---

## 8. Public API parity

Every method left in `Renderer.hpp` after §0 and §1. "Stub OK" means nothing in the repo calls it, but it's public API.

### Lifecycle / window
- [ ] `init(w, h, title, msaa)`: `valo::Window` + VALO. Ignore `msaa`.
- [ ] `ownsContext()`: always true now (or remove it along with `attach`)
- [ ] `shutdown()` / destructor: destroy textures, buffers and programs, `valo.shutdown()`, then the window
- [ ] `beginFrame()` / `endFrame()`: §5
- [ ] `getSize()` → `window.getSize()`
- [ ] `setTitle()` → `window.setTitle()`
- [ ] `setVsync()` / `getVsync()` → `valo.setVsync` / `getVsync`
- [ ] `setFramerateLimit()` / `getFramerateLimit()`: UILO's own limiter
- [ ] `getStats()`: `numDraw` = commands submitted, `numVertices` = arena vertex count, `cpuTimeMs` = time in `valo.endFrame`, `gpuTimeMs` = 0
- [ ] `setCursor()`: needs §2b #9

### Shapes (all go into the arena as ui-vertex batches)
- [ ] `draw(Rect)`: optional gradient drawn as a 5-vertex centre fan; outline = 4 rects inside the bounds
- [ ] `draw(RoundedRect)`: SDF mode, **not** a clip; outline = 4 `drawArc` corners + 4 edge rects, inset by the 0.5px arc fade
- [ ] `draw(Circle)`: SDF mode with radius = r, quad padded 1px
- [ ] `draw(Triangle)`: flat colour
- [ ] `draw(Line)`: quad with 1px alpha skirts. Don't use `Topology::Lines`; VALO's lines are 1px and aliased.
- [ ] `drawLines(lines, n)`: batched quads
- [ ] `drawArc(...)`: annulus with radial and angular skirts; innerR ≤ 0 draws a solid wedge

### Textures / images
- [ ] `loadTexture(path)`: keep UILO's stb decode (it needs width and height), then `createTexture(w, h, data)`. Cached by path; failures cached as invalid.
- [ ] `loadImagePixels(...)`: CPU only, unchanged
- [ ] `createTexture(w, h)` / `updateTexture(tex, rgba)`: the §2a workaround until VALO has `updateTexture`
- [ ] `destroyTexture(tex)` → `valo.destroyTexture`
- [ ] `drawImage(dst, tex, tint, uv, flipH, flipV, clipEllipse)`
- [ ] `Texture::handle` is uint16 and VALO's `TextureID` is uint32; assert the ID is below `UINT16_MAX`

### Text
- [ ] `loadFont`, `measureText`, `charPositions`: unchanged
- [ ] Glyph atlas: an RGBA8 VALO texture per (font, `round(size)`), re-uploaded at `endFrame` when dirty
- [ ] `drawText(...)`: ui text mode

### Clipping
- [ ] `pushScissor` / `popScissor`, `pushRoundClip` / `popRoundClip`: §7
- [ ] `clear(color)`: §5

### Framebuffers: none are called, stub OK
Later: `createTarget(w, h)` plus a layer, and `drawFrameBuffer` = a textured quad sampling the target ID.
- [ ] `createFrameBuffer` / `resizeFrameBuffer` / `destroyFrameBuffer` / `pushFrameBuffer` / `popFrameBuffer` / `drawFrameBuffer`

### Rotation: none are called, stub OK (or port; it's CPU-only)
- [ ] `setRotation` / `rotate` / `clearRotation`

---

## 9. Behaviours to reproduce exactly

**Coordinates and colour**
- [ ] Pixel space, origin top-left, framebuffer pixels. `valo::Window::mousePosition()` is already in pixels, so drop UILO's logical-to-pixel conversion for the mouse.
- [ ] `packColor` byte order (R,G,B,A in memory) matches VALO's ABGR32 texel format, so image and atlas bytes pass straight through
- [ ] VALO `Color` is float 0..1: convert `uilo::Color` / 255
- [ ] The scene clears to opaque black before the app's `clear()`

**Input**
- [ ] Held Backspace, Delete and arrow keys repeat in Textbox/Terminal
- [ ] Shift, Ctrl and Cmd shortcuts work exactly as today (the `handleKeyInput` flags)
- [ ] Text arrives as UTF-8 and is inserted at the caret; the key that produced it doesn't also trigger a command (today's key-up/timestamp suppression)
- [ ] Shift+wheel scrolls horizontally, Ctrl/Cmd+wheel zooms, trackpad horizontal scroll works
- [ ] Left and right presses are latched even when press and release land in the same frame (VALO's bitsets keep both)
- [ ] Cursor shape follows the hovered element

**Clipping**
- [ ] Clip bounds snapped to whole pixels (floor start, ceil end, clamp ≥ 0)
- [ ] A zero-area clip skips draws
- [ ] radius ≤ 0 means a plain rect clip

**Rotation**
- [ ] Applied on the CPU to vertices around a pivot; degrees; 0 = +x, 90 = +y; cleared at `beginFrame`

**Text**
- [ ] One atlas per (font, `round(sizePx)`), 1024², row packing with 1px padding; when full, glyphs silently don't draw
- [ ] Glyph quads snapped: `floor(pen + off + 0.5)`; baseline at `pos.y + ascent`; line advance `ascent + descent + lineGap`
- [ ] Fake bold draws each glyph twice, offset by `max(1, 0.04 * sizePx)`; fake italic shears by 0.21; neither changes the advances

**Images**
- [ ] UV sub-rect, flips swap UVs, tint multiplies, `clipEllipse` masks with AA

**Frame pacing**
- [ ] Limiter: sleep to 0.5 ms before the deadline, then spin; re-anchor if more than one interval behind

---

## 10. Simplifications (what disappears)

- [ ] **With §0 (materials):** the offscreen scene target, blur ladder and composite pass; the six reserved views; the bypass guess and deferred-glass replay; two of the five shader programs
- [ ] **With VALO rendering:** the depth/origin flip logic, the 16-bit index flushes, the duplicate `vs_tex` handles, the embedded-shader tables and the shaderc build step
- [ ] **With VALO's clip stack:** the scissor and round-clip stacks, clip uniforms, `clipVersion` cache, `applyClipUniforms` dedup and `batchStateMatches`/`captureBatchState`
- [ ] **With VALO's window:** the per-platform native-handle code in `Renderer::init` (HWND/Metal/X11/Wayland), the cursor cache, the SDL shim for the Wt build, and SDL in `build.sh`/CMake
- [ ] **One batching path** (the arena + pending command)
- [ ] **Remove duplication:** the frame-rate limiter copy-pasted twice in `endFrame`, and the ortho matrix built several times
- [ ] **Line AA:** give `drawLines` the same skirts as `draw(Line)`
- [ ] **`draw(Rect)` outline corners:** the outline rects overlap at the corners, so translucent outlines are darker there
- [ ] **Glyph atlases:** consider one shared atlas instead of 4 MB per (font, size)
- [ ] **Comments:** the `Desc:` blocks in `Renderer.cpp` that are just section dividers

---

## 11. Verification (software backend)

Run each example on the bgfx build (after §0) and the VALO build and compare.

| Example | Covers | Result |
|---|---|:-:|
| `containers` | rects, rounded clips, text, stats, vsync | [ ] |
| `gradients` | gradient fans; now on `ui.pollEvents()` | [ ] |
| `pixel_test` | pixel snapping, clip edges | [ ] |
| `contextmenu` | overlays, right-click, menu keyboard navigation | [ ] |
| `datepicker` | mixed shapes and text | [ ] |
| `theme` | system light/dark detection | [ ] |
| `render_bench` | batching: command count and frame time | [ ] |
| `text-editor` / Textbox | text input, key repeat, shortcuts, clipboard, caret, scissor clip | [ ] |
| `terminal` | key translation, clipboard, bold/italic | [ ] |
| Knob, Slider, Resizer, Waveform, Icon, Image | drag timing, `drawArc`, `drawLines`, texture updates | [ ] |

- [ ] Non-QWERTY layout (AZERTY/Dvorak): Ctrl+Z/C/V/A do what the user expects (§2b #3)
- [ ] Fast typing: no dropped or reordered characters (§2b #4)
- [ ] Live window resize keeps redrawing (§2b #10)
- [ ] macOS: trackpad scroll momentum and pinch zoom (§2b #11)
- [ ] Nested rounded clips 3+ deep
- [ ] Scrolled content inside a rounded, clipped container
- [ ] HiDPI window; move between monitors with different scales
- [ ] Minimised window (0×0; VALO TODO flags zero-size targets as UB)
- [ ] Vsync toggle at runtime
- [ ] Clip stack deeper than 64
- [ ] Atlas fills up / very long text
- [ ] ThreadSanitizer run (shaders on worker threads)
- [ ] Wt (`UILO_WT`) build still links
- [ ] `grep -rn "SDL_\|<SDL3" include` finds nothing outside any deliberate §2b fallback
- [ ] Remove bgfx and SDL from `ext/`, `build.sh`, `CMakeLists.txt`, `cmake/bgfxToolUtils.cmake`, the `.sc` shaders + shaderc step, and the `uilo-new` scaffold

---

## 12. When VALO's Vulkan backend lands

UILO code shouldn't change. Re-check these:

- [ ] The ui (and clip) program has its GLSL/SPIR-V twin via VALO's shader contract (VALO TODO §3)
- [ ] Per-command uniforms fit whatever VALO maps `setUniforms` to (UI uniforms are ~16 B)
- [ ] Re-run all of §11 on Vulkan
- [ ] Delete the §2a workarounds once `updateTexture`, R8 and scissor are native
