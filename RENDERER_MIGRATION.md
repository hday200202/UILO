# Migration: bgfx + SDL → VALO

Checklist for moving UILO onto VALO ([ext/VALO](ext/VALO)). VALO replaces bgfx
(rendering) **and** SDL (window, input, platform). When this is done, UILO has no
direct dependency on either, and nothing from SDL appears in UILO's public API.

Everything VALO needed for this is in place: the six items in
[ext/VALO/UILO_BLOCKERS.md](ext/VALO/UILO_BLOCKERS.md) are done and verified
(per-frame updates without stalls, embedded shaders, the window/input surface,
subproject + MSVC builds, matching blend on both backends, dynamic stencil
reference). What's left is UILO-side work.

UILO's public API stays the same apart from two intended changes:
1. **The material/glass system is removed (§0).**
2. **SDL types are removed from the public API (§2).**

VALO has two backends, software and Vulkan, chosen at runtime. Both run the same
UILO code; §12 is the per-backend verification.

---

## How bgfx/SDL concepts map onto VALO

| bgfx / SDL (today) | VALO |
|---|---|
| `SDL_CreateWindow` + `SDL_Init`/`SDL_Quit` | `valo::Window(title, size, WinFlags::Resize \| WinFlags::HiDPI [\| WinFlags::Vulkan])` |
| `SDL_PollEvent` loop → `UILO::handleEvent` | `window.pollEvents()`, then walk `window.events()` (ordered `WindowEvent`s) |
| `SDL_GetKeyboardState` / `SDL_GetMouseState` | `window.isKeyDown(Key)` / `isMouseDown(MouseButton)` / `mousePosition()` (pixels) |
| `SDL_Scancode` / `SDLK_*` | `valo::Key` (scancode) + `WindowEvent::symbol` (layout character) |
| `SDL_GetModState` / `SDL_KMOD_*` | `window.modState()` / `WindowEvent::mods`, `valo::KeyMod` |
| `SDL_EVENT_TEXT_INPUT`, `SDL_StartTextInput` | `WindowEventType::Text` (`text[32]`), `startTextInput()` / `stopTextInput()` |
| `SDL_Get/SetClipboardText` | `window.getClipboardText()` / `setClipboardText()` |
| `SDL_CreateSystemCursor` / `SDL_SetCursor` | `window.setCursor(CursorShape)` |
| `SDL_SetWindowRelativeMouseMode` | `grabMouse()` / `releaseMouse()` / `isMouseGrabbed()` |
| `SDL_AddEventWatch` (live resize) | `window.setResizeCallback(fn)` |
| `SDL_GetPointerProperty(... COCOA_WINDOW ...)` | `window.nativeHandle()` |
| `SDL_GetWindowDisplayScale`, refresh rate, `SDL_GetSystemTheme`, base/pref path | `getDisplayScale()`, `getRefreshRate()`, `Window::getSystemTheme()`, `Window::basePath()` / `prefPath()` |
| `SDL_GetTicks` | `valo::Clock` (or `std::chrono`) |
| `bgfx::init` | `VALO::init(window, cfg)` with `cfg.backend = Backend::Software / Vulkan` |
| `bgfx::frame()` | `VALO::endFrame()` |
| `bgfx::reset` on resize | Automatic: `VALO::beginFrame` checks `window.resized()` |
| Scene view + offscreen FB + composite | `BASE_LAYER` drawing straight into `BACKBUFFER` |
| Framebuffer / render target | `createTarget(w, h)`; targets and textures share one ID space |
| Transient VB/IB | One persistent **arena** VB/IB, `updateVertexBuffer`/`updateIndexBuffer` once per frame (cheap on both backends now) |
| `setState` blend | `RenderCommand::blendmode` (`Alpha` = standard "over", same on both backends) |
| `setScissor` | `RenderCommand::scissor` |
| Rounded-clip uniforms | `VALO::pushClip(shapeCmd)` / `popClip()` (stencil, nests to any depth) |
| `setUniform` vec4s | `RenderCommand::setUniforms(struct)` (**≤ 128 B** to run on Vulkan) |
| `.sc` shaders + shaderc + embedded headers | `.vert.hpp`/`.frag.hpp` C++ shaders, `createProgram(vsPath, fsPath)`, `valo_embed_shaders()` in CMake |
| R8 glyph atlas | RGBA8 texture (VALO has no R8 yet) |
| `bgfx::getStats()` | None. Count commands in UILO and time `endFrame` |
| 16-bit indices | 32-bit indices |

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

## 1. Bring VALO into the build

- [ ] **Track VALO properly.** `ext/VALO` is currently an untracked clone in UILO's tree. Make it a git submodule (or vendor it at a pinned commit) so UILO builds against a known VALO.
- [ ] **Fetch VALO's own dependencies.** VALO's `build.sh` fetches SDL3 (`ext/SDL3-install`), stb, tinyobjloader, cgltf, VMA and the Vulkan SDK (which provides `glslc` for `valo_embed_shaders`). UILO's `build.sh` runs it, or does the same fetches.
- [ ] `add_subdirectory(ext/VALO)` and link `VALO::VALO`. As a subproject VALO builds no examples, no valoc, and writes nothing into its own `lib/`.
- [ ] **One SDL3.** Either let VALO find its own `ext/SDL3-install` (UILO stops vendoring SDL entirely), or have UILO define `SDL3::SDL3-static` before `add_subdirectory` and VALO will use it. Pick one; never both.
- [ ] **MSVC runtime:** VALO leaves `CMAKE_MSVC_RUNTIME_LIBRARY` to the parent. UILO forces `/MT` today because of bgfx (CMakeLists ~line 49); once bgfx is gone, pick one runtime for the whole build.
- [ ] **stb:** VALO defines `STB_IMAGE_IMPLEMENTATION` and exposes its `ext/stb` include dir publicly. Remove the define from UILO's `Renderer_Texture.cpp` and UILO's copy of `stb_image.h`. UILO keeps `stb_truetype` (VALO has no font code).
- [ ] VALO's `software/Sampling.hpp` defines `discard` as a macro (it pre-includes `<random>` so the standard library is safe). Only UILO's renderer `.cpp` files and its shader headers should include VALO render headers. Public UILO headers may include `render/Window.hpp` (for `Key`, `MouseButton`, `KeyMod`, `CursorShape`); it forward-declares its SDL types and includes no SDL header.
- [ ] `valo::Color` (float `Vec4f`), `valo::Vec2f` and friends clash with `uilo::Color` / `uilo::Vec2f`. Don't `using namespace valo`.
- [ ] **Wt build (`UILO_WT`):** it still links no renderer. If public headers include `render/Window.hpp` for the key types, the Wt build needs VALO's include path but not the library. Delete `wt/shim/SDL3/SDL.h` and the SDL stubs in `HeadlessBackend.cpp` once nothing in UILO calls SDL.

---

## 2. Remove SDL from UILO's public API

**Decisions first:**
- [ ] **Key types in UILO's API:** re-export as `uilo::Key = valo::Key`, `uilo::MouseButton`, `uilo::KeyMod` (recommended, since it keeps VALO's names out of user code), or use `valo::` directly.
- [ ] **Event model:** drop `UILO::handleEvent(const SDL_Event&)`. `UILO::pollEvents()` calls `window.pollEvents()` and walks `window.events()`. VALO's ordered event list carries everything `handleEvent` used (order, timestamps, mods, press position, repeat, text), so the logic ports one-to-one. If you want custom loops back later, expose a UILO-owned event type; don't expose `valo::WindowEvent` directly.
- [ ] **Window access:** remove `Renderer::sdlWindow()`. UILO's .cpp files get the `valo::Window&` through an internal accessor.

**Public headers to change:**
- [ ] [UILO.hpp](include/UILO.hpp): remove `handleEvent(const SDL_Event&)`; update the `pollEvents` / `setRenderer` / `queryMousePixelPosition` comments
- [ ] [Renderer.hpp](include/renderer/Renderer.hpp): remove `struct SDL_Window;`, `sdlWindow()`, `m_window`, `attach(SDL_Window*, uint16_t)` (no callers; VALO owns its window), and the bgfx leftovers (`namespace bgfx { … }`, `FrameBuffer::viewId`, `m_nextViewId`, `m_viewStack*`, `currentViewId()`, `submitOrtho()`, `m_resetFlags`). Fix the bgfx wording in the `Texture`, `RendererStats` and `FrameBuffer` comments.
- [ ] [input/Keybinds.hpp](include/input/Keybinds.hpp): `SDL_Scancode` → `Key`; `SDL_GetKeyboardState` → `window.isKeyDown`. It's header-only, so pass the window in or move the implementation to a .cpp.
- [ ] [input/Mousebinds.hpp](include/input/Mousebinds.hpp): `SDL_Window*` → the window; button masks → `MouseButton`; relative mode → `grabMouse`/`releaseMouse`; relative delta → `mouseDelta()`, which is already in pixels, so drop the extra `SDL_GetWindowDisplayScale` multiply.
- [ ] [Interactible.hpp](include/elements/interactible/Interactible.hpp): `handleKeyInput(SDL_Keycode key, bool shift, bool ctrl, bool gui)` → `handleKeyInput(Key key, char32_t symbol, KeyMod mods)` (or keep the bool flags). Named keys (arrows, Return, Backspace, F-keys) match on `Key`; letter shortcuts (Ctrl+Z/C/V/A/X/Y) match on `symbol`, so they follow the keyboard layout as `SDLK_*` did.
- [ ] [Textbox.hpp](include/elements/interactible/Textbox.hpp), [Terminal.hpp](include/elements/widgets/Terminal.hpp): the `handleKeyInput` and `sendKey(SDL_Keycode, …)` signatures
- [ ] [Knob.hpp](include/elements/interactible/Knob.hpp), [Slider.hpp](include/elements/interactible/Slider.hpp): drop the `<SDL3/SDL.h>` include

**Implementation files that use SDL directly:**
- [ ] `UILO.cpp`: event handling (§4), text input start/stop, modifiers, the live-resize event watch → `setResizeCallback`, the Cocoa window lookup → `nativeHandle()`
- [ ] `Textbox.cpp` (16 `SDLK_*` uses), `Terminal.cpp` (22): key handling → `Key` + `symbol`; `SDL_GetModState` → `modState()`; clipboard → `get/setClipboardText`
- [ ] `Canvas.cpp`: `SDL_GetModState` → `modState()`
- [ ] `Container.cpp`: `SDL_GetMouseState` + left mask during a drag → `isMouseDown(MouseButton::Left)` / `mousePosition()`
- [ ] `Knob.cpp`, `Slider.cpp`, `Resizer.cpp`: `SDL_GetTicks` → `valo::Clock` / `std::chrono`
- [ ] `utils/OS.cpp`:
  - display scale → `getDisplayScale()`
  - refresh rate → `getRefreshRate()`
  - display size → `getMonitorSize()`
  - theme → `Window::getSystemTheme()`
  - paths → `Window::basePath()` / `prefPath()`
  - CPU count → `std::thread::hardware_concurrency()`
  - RAM → a native API (VALO has none)

  The functions that run before a window exists (primary-display queries) need a window, or keep a small native implementation. Check each caller.
- [ ] `renderer/*`: replaced by the port (§5–§9)
- [ ] **Examples** with their own `SDL_PollEvent` loop (`gradients`, `pixel_test`, `render_bench`): switch to `ui.pollEvents()`. Remove the SDL includes from `containers`, `contextmenu`, `datepicker`, `terminal`, `theme`.
- [ ] `tools/ctxmenu_probe.cpp`: it synthesises input with `SDL_PushEvent`, which VALO has no equivalent for. Rework it or retire it.
- [ ] Verify with `grep -rn "SDL" include` returning only comments you mean to keep

---

## 3. Window setup

- [ ] `Renderer::init` builds a `valo::Window(title, {w, h}, WinFlags::Resize | WinFlags::HiDPI | (vulkan ? WinFlags::Vulkan : None))`. The Vulkan backend needs `WinFlags::Vulkan` on the window, so choose the backend before creating it.
- [ ] Keep `respectOSScale = false` (the default): sizes are in pixels, matching how UILO works today. `getSize()` and `mousePosition()` are already in pixels, so delete UILO's logical-to-pixel mouse conversion.
- [ ] **Backend selection:** keep the existing `UILO_RENDERER` environment variable, with values `software` and `vulkan`, and drop `metal`, `d3d11`, `d3d12` and `gl`. Decide the default: Vulkan when available, falling back to software if `VALO::init` fails, is the friendliest.
- [ ] Shutdown order: `valo.shutdown()`, then let the `valo::Window` destruct (it quits SDL when it's the last window).
- [ ] Set `RenderConfig::validation = true` in debug builds while porting; it now includes synchronization validation.

---

## 4. Input loop

```
ui.pollEvents()       → window.pollEvents(); walk window.events(); quit on CloseRequested / false return
ui.update()           → layout, hover, clicks, key dispatch
renderer.beginFrame() → valo.beginFrame()   (resize, clears backbuffer + stencil)
renderer.clear(c)     → §6
ui.render()           → shapes into the arena, clips (§8)
renderer.endFrame()   → flush, upload arena, valo.endFrame(), frame limiter
```

- [ ] `KeyDown` → context-menu navigation, then the focused element's `handleKeyInput(key, symbol, mods)`. Repeats arrive as `KeyDown` with `repeat = true`, so held Backspace and arrows keep working.
- [ ] `Text` → focused element (UTF-8 in `text`). Port the existing "text that arrives before the key's key-up is a command, not input" check using `timestamp`; the timestamps are nanoseconds, as `SDL_Event::common.timestamp` was.
- [ ] `MouseDown` / `MouseUp` → pending press with `mouse` as the press position; `clicks` gives double-click for free
- [ ] `Wheel` → `dispatchScroll(mouse, wheel, precise)`; Shift swaps axes and Ctrl/Cmd zooms, using the event's `mods` as before
- [ ] `FocusLost` → release drags and held-key state
- [ ] Focus moving into or out of a Textbox/Terminal → `startTextInput()` / `stopTextInput()`
- [ ] `setResizeCallback` → the existing `m_onLiveResize` hook. It runs inside the OS's modal resize loop and also sets `resized()`, so VALO picks up the new size there.
- [ ] macOS: `configureMacWindowForLiveResize`, `installMacScrollMonitor` and `installMacZoomMonitor` take `nativeHandle()` (an `NSWindow*`), unchanged otherwise
- [ ] Keybinds / Mousebinds read from the same window

---

## 5. Frame structure (rendering)

UILO uses one layer: VALO's `BASE_LAYER`, drawing into `BACKBUFFER`.

**`init`:**
- [ ] `VALO::init(window, cfg)` with `cfg.backend` and `cfg.vsync`
- [ ] The arena VB/IB (`createVertexBuffer` needs initial data: create with one dummy vertex)
- [ ] Programs (§7)

**`beginFrame`:**
- [ ] `valo.beginFrame()`: resizes if needed, clears colour/depth/stencil to `BASE_LAYER`'s clear colour, and retires destroyed resources
- [ ] Reset the arena, pending batch, stats and rotation

**Drawing:**
- [ ] Shapes append vertices and indices to CPU-side arena vectors, then extend or start a pending command covering that index range (`firstIndex` / `indexCount`).
- [ ] **Upload the arena once, just before `valo.endFrame`.** Commands reference buffers by ID and run at `endFrame`, so the last update of a frame is what every draw that frame reads. On Vulkan this is now a `memcpy` into a per-frame mapped slot, with no stall.
- [ ] Command template:
  - `topology = Triangles`, `cullmode = None`
  - `depthTest = false`, `depthWrite = false` (VALO defaults to reversed-Z depth on, which UI doesn't want)
  - `blendmode = Alpha`
  - always set `program`
- [ ] **Merge consecutive draws** that share program, uniforms (including the texture ID) and scissor into one command. Software no longer pays per draw, but Vulkan still rebinds per command, and fewer commands is simpler anyway.
- [ ] **Flush the pending batch before every `pushClip`/`popClip` and whenever the scissor changes.** `pushClip` enqueues immediately, and `submit` stamps the stencil test at submit time.

**`endFrame`:**
- [ ] Flush the pending batch → upload the arena → `valo.endFrame()` → frame limiter (UILO's sleep+spin; `RenderConfig::maxFps` is not used by VALO)

**`clear(color)`:**
- [ ] VALO clears in `beginFrame`, before UILO's `clear()` runs. So:
  - `setLayerClearColor(BASE_LAYER, c)` for the following frames
  - **and**, if `c` differs from what was just cleared, submit a fullscreen `Opaque` quad for this frame

---

## 6. Textures and text

- [ ] `loadTexture(path)`: keep UILO's cache and its stb decode (it needs width and height), then `createTexture(w, h, data, srgb = false, mips = true)`. Mips are fine for static images and fix downscaled shimmer.
- [ ] `createTexture(w, h)` + `updateTexture(tex, rgba)` (Icon, Image): `createTexture(w, h, zeroed, false, mips = false)`, then `valo.updateTexture(id, data)`. **Pass `mips = false` for anything updated**, otherwise each update rebuilds the mip chain.
- [ ] `destroyTexture` → `valo.destroyTexture` (destruction is deferred until no frame in flight uses it, and the ID is not reused before then)
- [ ] Keep `srgb = false` everywhere and `RenderConfig::srgbBackbuffer = false`. bgfx blended in gamma space, and matching that keeps every colour identical.
- [ ] **Glyph atlas:** an RGBA8 texture per (font, `round(size)`), 1024², `mips = false`, white RGB with coverage in alpha (no R8 in VALO yet; 4 MB per atlas). Each new glyph → `valo.updateTextureRect(id, x, y, w, h, rgba)`. On Vulkan these are recorded into the frame's upload command buffer, so many per frame are fine.
- [ ] `Texture::handle` is uint16 and VALO's `TextureID` is uint32; assert below `UINT16_MAX`
- [ ] Sampling: `texture(id, uv, Sampler(Wrap::Clamp, Wrap::Clamp, Filter::Linear))`. Texture IDs are bindless on Vulkan, so the ID lives in the uniforms (or the vertex) rather than being bound.

---

## 7. Shaders

The sources to port are `vs_solid`, `vs_tex`, `fs_solid`, `fs_tex` and `fs_text` in [include/renderer/shaders/](include/renderer/shaders/). [ext/VALO/examples/shaders/roundrect.vert.hpp](ext/VALO/examples/shaders/roundrect.vert.hpp) / `.frag.hpp` is the template: it's a UI rounded rect, and it's what `clipstack` embeds.

**Format:** one `ui.vert.hpp` and one `ui.frag.hpp` (the frag includes the vert, so they share structs), each ending with `VALO_VERTEX_SHADER(fn)` / `VALO_FRAGMENT_SHADER(fn)`. Shared helpers such as the SDF function can live in a plain project header; valoc now translates only the helpers a shader actually reaches.

```cpp
struct UiVertex {                 // VtxLayout({Vec2f, Vec2f, Vec4f, Vec4f, Vec4f})
    Vec2f pos;                    // pixels
    Vec2f uv;
    Vec4f color;                  // linear 0..1 (or Uint32 + unpack; must be a GPU-legal type)
    Vec4f shape;                  // SDF rect: cx, cy, halfW, halfH (pixels)
    Vec4f params;                 // x = radius, y = mode (solid/rrect/tex/text/ellipse), zw spare
};
struct UiUniforms { Vec2f viewport; TextureID tex; uint32_t pad; };   // 16 B, far under Vulkan's 128
```

- [ ] **Rules valoc enforces:** no `bool` in GPU-read structs (use `uint32_t` flags), varyings are floats only, buffer slots are compile-time constants, and integers used as conditions are compared explicitly (`x != 0u`)
- [ ] **ui vs:** pixel → NDC from `viewport`, using VALO's convention (clip space is Y-up, as in `clipstack.cpp`'s `Mat4f::orthographic(0, w, h, 0, -1, 1)`), and pass the varyings through
- [ ] **ui fs modes:**
  - solid: vertex colour (line and arc AA comes from vertex-alpha skirts, unchanged)
  - rounded rect / circle: rounded-box SDF, `alpha *= 1 - smoothstep(-0.5, 0.5, d)` (d is already in pixels, so no `fwidth` is needed), `discard` at alpha 0
  - texture: sample × tint
  - text: atlas alpha × colour
  - ellipse: texture + analytic ellipse mask
- [ ] **clip program:** the rounded-box SDF that `discard`s outside, for `pushClip` shapes (VALO turns `colorWrite` off itself). It can be the ui program.
- [ ] **Program creation:** `valo.createProgram("ui.vert.hpp", "ui.frag.hpp")`, the same call on both backends. Software resolves it through the shader registry, and Vulkan uses the embedded SPIR-V. It refuses a pair whose Uniforms or Varyings differ.
- [ ] **CMake:** `valo_embed_shaders(uilo SHADERS include/renderer/shaders/ui.vert.hpp include/renderer/shaders/ui.frag.hpp [INCLUDES …])`
- [ ] **Commit the generated `ui.vert` / `ui.frag` GLSL** beside the `.hpp`s. valoc (which needs libclang) only runs where it was built; everywhere else the embed step compiles the committed GLSL. If UILO's `.gitignore` ever ignores `*.vert` / `*.frag` the way VALO's does, add exceptions for these.
- [ ] Delete the `.sc` files, `varying.def.sc`, the shaderc step, `cmake/bgfxToolUtils.cmake` and the `spirv/glsl/essl/metal/dxbc` header includes

---

## 8. Clipping

Two mechanisms now, chosen per clip:

- [ ] **Rect clips → `RenderCommand::scissor`.** `pushScissor`, and `pushRoundClip` with radius ≤ 0, keep UILO's own scissor stack (intersection + floor/ceil pixel snapping, as today), and the current top goes into every command's `scissor`. That's free on both backends, with no stencil passes.
- [ ] **Rounded clips → `valo.pushClip(roundedRectCmd)` / `popClip()`.** These are stencil-based and nest exactly to any depth; nesting no longer creates pipelines. Also push the rounded clip's bounds onto the scissor stack, so geometry outside them is rejected early.
- [ ] **pushRoundClip = one VALO push and popRoundClip = one VALO pop.** Keep UILO's own counts balanced, the 64-deep cap, and overflow counting (the stencil is 8-bit).
- [ ] A zero-area scissor skips draws entirely (today's `scissorEmpty`)
- [ ] **Stop using clips for a shape's own mask.** `draw(RoundedRect)` and `draw(Circle)` use the SDF mode instead.
- [ ] The scissor is not part of VALO's clip stack, so remember that `pushClip`'s own stencil draw also needs the current scissor set on its command
- [ ] **Behaviour changes to check:**
  - every ancestor rounded clip now applies, not just the two innermost
  - children spilling past a rounded corner get a hard stencil edge rather than SDF AA (the panel's own background is still AA'd)

---

## 9. Public API parity

Every method left in `Renderer.hpp` after §0 and §2. "Stub OK" means nothing in the repo calls it, but it's public API.

### Lifecycle / window
- [ ] `init(w, h, title, msaa)`: §3. Ignore `msaa`.
- [ ] `ownsContext()`: always true, or remove it along with `attach`
- [ ] `shutdown()` / destructor: §3
- [ ] `beginFrame()` / `endFrame()`: §5
- [ ] `getSize()` → `window.getSize()`
- [ ] `setTitle()` → `window.setTitle()`
- [ ] `setVsync()` / `getVsync()` → `valo.setVsync` / `getVsync`
- [ ] `setFramerateLimit()` / `getFramerateLimit()`: UILO's limiter
- [ ] `getStats()`: `numDraw` = commands submitted, `numVertices` = arena vertices, `cpuTimeMs` = time in `valo.endFrame`, `gpuTimeMs` = 0
- [ ] `setCursor(CursorType)` → `window.setCursor(CursorShape)`: Arrow→Arrow, Hand→Hand, SizeHorizontal→ResizeEW, SizeVertical→ResizeNS, Text→Text, Crosshair→Crosshair

### Shapes (into the arena)
- [ ] `draw(Rect)`: optional gradient drawn as a 5-vertex centre fan; outline = 4 rects inside the bounds
- [ ] `draw(RoundedRect)`: SDF mode; outline = 4 `drawArc` corners + 4 edge rects, inset by the 0.5px arc fade
- [ ] `draw(Circle)`: SDF mode with radius = r, quad padded 1px
- [ ] `draw(Triangle)`: flat colour
- [ ] `draw(Line)`: quad with 1px alpha skirts. Don't use `Topology::Lines` (1px, aliased).
- [ ] `drawLines(lines, n)`: batched quads
- [ ] `drawArc(...)`: annulus with radial and angular skirts; innerR ≤ 0 draws a solid wedge

### Textures / images / text
- [ ] `loadTexture`, `loadImagePixels`, `createTexture`, `updateTexture`, `destroyTexture`, `drawImage`: §6
- [ ] `loadFont`, `measureText`, `charPositions`: unchanged. `drawText`: §6 and the ui text mode.

### Clipping
- [ ] `pushScissor` / `popScissor`, `pushRoundClip` / `popRoundClip`: §8
- [ ] `clear(color)`: §5

### Framebuffers: none are called, stub OK
Implementable later: `createTarget(w, h)` + a layer targeting it, and `drawFrameBuffer` = a textured quad sampling the target ID. Blend alpha now matches across backends, so compositing a target is consistent. VALO's `submit` stamps the current clip's stencil test on every layer, so switch layers only with the clip stack empty.
- [ ] `createFrameBuffer` / `resizeFrameBuffer` / `destroyFrameBuffer` / `pushFrameBuffer` / `popFrameBuffer` / `drawFrameBuffer`

### Rotation: none are called, stub OK (or port; it's CPU-only)
- [ ] `setRotation` / `rotate` / `clearRotation`

---

## 10. Behaviours to reproduce exactly

**Coordinates and colour**
- [ ] Pixel space, origin top-left, framebuffer pixels
- [ ] `uilo::Color` (uint8) → `valo::Color` / vertex colour as `/ 255`, with no sRGB conversion anywhere
- [ ] The scene clears to opaque black before the app's `clear()`

**Input**
- [ ] Held Backspace, Delete and arrows repeat in Textbox/Terminal
- [ ] Shift / Ctrl / Cmd shortcuts behave as today, and Ctrl+Z/C/V/A follow the keyboard layout (`symbol`)
- [ ] Typed text (including non-ASCII and IME input) is inserted at the caret, and the key that produced it doesn't also trigger a command
- [ ] Shift+wheel scrolls horizontally, Ctrl/Cmd+wheel zooms, trackpad horizontal scroll and momentum work
- [ ] Double-click selects a word in Textbox (now available via `clicks`, if UILO wants it)
- [ ] The cursor shape follows the hovered element

**Clipping**
- [ ] Clip bounds snapped to whole pixels (floor start, ceil end, clamp ≥ 0)
- [ ] A zero-area clip skips draws; radius ≤ 0 means a rect (scissor) clip

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

## 11. Simplifications (what disappears)

- [ ] **With §0 (materials):** the offscreen scene target, blur ladder and composite pass; the six reserved views; the bypass guess and deferred-glass replay; two shader programs
- [ ] **With VALO rendering:** per-API shader builds and embedded-shader tables (replaced by one `.hpp` pair and `valo_embed_shaders`), the depth/origin flip logic, the 16-bit index flushes, the transient-buffer budgets
- [ ] **With VALO clipping:** the round-clip stack, clip uniforms, `clipVersion` cache, `applyClipUniforms` dedup, `batchStateMatches`/`captureBatchState`, and the two-clip limit
- [ ] **With VALO's window:** the native-handle code in `Renderer::init` (HWND/Metal/X11/Wayland), the cursor cache, the SDL shim for the Wt build, and SDL in UILO's `build.sh`/CMake
- [ ] **One batching path** (the arena + pending command)
- [ ] **Remove duplication:** the frame-rate limiter copy-pasted twice in `endFrame`, and the ortho matrix built several times
- [ ] **Line AA:** give `drawLines` the same skirts as `draw(Line)`
- [ ] **`draw(Rect)` outline corners:** the outline rects overlap at the corners, so translucent outlines are darker there
- [ ] **Glyph atlases:** consider one shared atlas instead of 4 MB per (font, size)
- [ ] **Comments:** the `Desc:` blocks in `Renderer.cpp` that are just section dividers

---

## 12. Verification

Run each on the bgfx build (after §0) and on VALO's **software** and **Vulkan** backends.

| Example | Covers | Software | Vulkan |
|---|---|:-:|:-:|
| `containers` | rects, rounded clips, text, stats, vsync | [ ] | [ ] |
| `gradients` | gradient fans; now on `ui.pollEvents()` | [ ] | [ ] |
| `pixel_test` | pixel snapping, scissor edges | [ ] | [ ] |
| `contextmenu` | overlays, right-click, menu keyboard navigation | [ ] | [ ] |
| `datepicker` | mixed shapes and text | [ ] | [ ] |
| `theme` | system light/dark detection | [ ] | [ ] |
| `render_bench` | batching: command count and frame time | [ ] | [ ] |
| `text-editor` / Textbox | text input, repeat, shortcuts, clipboard, caret, scissor | [ ] | [ ] |
| `terminal` | key translation, clipboard, bold/italic | [ ] | [ ] |
| Knob, Slider, Resizer, Waveform, Icon, Image | drag timing, `drawArc`, `drawLines`, texture updates | [ ] | [ ] |

- [ ] Non-QWERTY layout (AZERTY/Dvorak): Ctrl+Z/C/V/A do what the user expects
- [ ] **A real IME composition** (Japanese/Chinese) in a Textbox. VALO's own test couldn't exercise this.
- [ ] **Shift+wheel horizontal scroll.** Also untested in VALO's harness.
- [ ] Fast typing: no dropped or reordered characters
- [ ] Live window resize keeps redrawing (Windows and macOS)
- [ ] macOS: trackpad scroll momentum and pinch zoom through `nativeHandle()`
- [ ] Nested rounded clips 3+ deep; scrolled content inside a rounded container
- [ ] HiDPI window; move between monitors with different scales
- [ ] Minimised window (0×0)
- [ ] Vsync toggle at runtime
- [ ] Clip stack deeper than 64
- [ ] Atlas fills up / very long text; new glyphs appearing while typing on Vulkan (validation clean)
- [ ] Vulkan with `validation = true`: zero messages across all examples
- [ ] A built example runs from an unrelated directory with no shader files or tools beside it
- [ ] ThreadSanitizer run (software shaders run on worker threads, so they must not touch shared state)
- [ ] Wt (`UILO_WT`) build still builds and links
- [ ] `grep -rn "SDL_\|<SDL3\|bgfx" include` finds nothing
- [ ] Remove bgfx and SDL from `ext/`, `build.sh`, `CMakeLists.txt`, `cmake/bgfxToolUtils.cmake` and the `uilo-new` scaffold. Point the scaffold at VALO instead.
