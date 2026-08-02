# HeadlessBackend.cpp

`include/wt/HeadlessBackend.cpp`

[← index](../README.md)

## Types

- [Pty](#pty)

## Functions

- [`SDL_GetMouseState(float* x, float* y)`](#sdl-getmousestate)
- [`SDL_GetTicks()`](#sdl-getticks)
- [`SDL_GetModState()`](#sdl-getmodstate)
- [`SDL_GetKeyboardState(int* numkeys)`](#sdl-getkeyboardstate)
- [`SDL_GetClipboardText()`](#sdl-getclipboardtext)
- [`SDL_SetClipboardText(const char*)`](#sdl-setclipboardtext)
- [`SDL_free(void*)`](#sdl-free)
- [`SDL_StartTextInput(SDL_Window*)`](#sdl-starttextinput)
- [`SDL_StopTextInput(SDL_Window*)`](#sdl-stoptextinput)
- [`SDL_TextInputActive(SDL_Window*)`](#sdl-textinputactive)
- [`SDL_AddEventWatch(SDL_EventFilter, void*)`](#sdl-addeventwatch)
- [`SDL_PollEvent(SDL_Event*)`](#sdl-pollevent)
- [`SDL_SetWindowRelativeMouseMode(SDL_Window*, bool)`](#sdl-setwindowrelativemousemode)
- [`SDL_GetWindowRelativeMouseMode(SDL_Window*)`](#sdl-getwindowrelativemousemode)
- [`SDL_GetRelativeMouseState(float* x, float* y)`](#sdl-getrelativemousestate)
- [`SDL_GetWindowDisplayScale(SDL_Window*)`](#sdl-getwindowdisplayscale)
- [`SDL_GetError()`](#sdl-geterror)
- [`SDL_WasInit(SDL_InitFlags)`](#sdl-wasinit)
- [`SDL_GetPrimaryDisplay()`](#sdl-getprimarydisplay)
- [`SDL_GetDisplayContentScale(SDL_DisplayID)`](#sdl-getdisplaycontentscale)
- [`SDL_GetDisplayBounds(SDL_DisplayID, SDL_Rect* rect)`](#sdl-getdisplaybounds)
- [`SDL_GetCurrentDisplayMode(SDL_DisplayID)`](#sdl-getcurrentdisplaymode)
- [`SDL_GetDisplays(int* count)`](#sdl-getdisplays)
- [`SDL_GetSystemTheme()`](#sdl-getsystemtheme)
- [`SDL_GetNumLogicalCPUCores()`](#sdl-getnumlogicalcpucores)
- [`SDL_GetSystemRAM()`](#sdl-getsystemram)
- [`SDL_GetBasePath()`](#sdl-getbasepath)
- [`SDL_GetPrefPath(const char*, const char*)`](#sdl-getprefpath)
- [`SDL_GetWindowSize(SDL_Window*, int* w, int* h)`](#sdl-getwindowsize)
- [`SDL_GetWindowSizeInPixels(SDL_Window*, int* w, int* h)`](#sdl-getwindowsizeinpixels)
- [`SDL_GetWindowProperties(SDL_Window*)`](#sdl-getwindowproperties)
- [`SDL_GetPointerProperty(SDL_PropertiesID, const char*, void* default_value)`](#sdl-getpointerproperty)
- [`configureMacWindowForLiveResize(void*)`](#configuremacwindowforliveresize)
- [`installMacScrollMonitor(std::function&lt;bool(float, float, bool)&gt;)`](#installmacscrollmonitor)
- [`installMacZoomMonitor(std::function&lt;bool(float)&gt;)`](#installmaczoommonitor)
- [`tickMacScrollMomentum(float)`](#tickmacscrollmomentum)
- [`cancelMacScrollMomentum()`](#cancelmacscrollmomentum)
- [`Renderer()`](#renderer)
- [`~Renderer()`](#renderer)
- [`init(uint32_t width, uint32_t height, const std::string&, uint8_t msaa)`](#init)
- [`attach(SDL_Window*, uint16_t)`](#attach)
- [`shutdown()`](#shutdown)
- [`beginFrame()`](#beginframe)
- [`endFrame()`](#endframe)
- [`getSize()`](#getsize)
- [`setTitle(const std::string&)`](#settitle)
- [`setVsync(bool)`](#setvsync)
- [`getVsync()`](#getvsync)
- [`setFramerateLimit(float)`](#setframeratelimit)
- [`getFramerateLimit()`](#getframeratelimit)
- [`getStats()`](#getstats)
- [`setCursor(CursorType)`](#setcursor)
- [`draw(const Rect&)`](#draw)
- [`draw(const RoundedRect&)`](#draw)
- [`draw(const Circle&)`](#draw)
- [`draw(const Triangle&)`](#draw)
- [`draw(const Line&)`](#draw)
- [`drawLines(const Line*, size_t)`](#drawlines)
- [`drawArc(Vec2f, float, float, float, float, Color, int)`](#drawarc)
- [`loadTexture(const std::string&)`](#loadtexture)
- [`destroyTexture(Texture& tex)`](#destroytexture)
- [`loadImagePixels(const std::string&, std::vector&lt;uint8_t&gt;&, uint32_t&, uint32_t&)`](#loadimagepixels)
- [`createTexture(uint16_t, uint16_t)`](#createtexture)
- [`updateTexture(const Texture&, const uint8_t*)`](#updatetexture)
- [`drawImage(const Rectf&, const Texture&, Color, Rectf, bool, bool, bool)`](#drawimage)
- [`drawGlass(const Rectf&, const Material&, Color)`](#drawglass)
- [`setMouseState(Vec2f pos)`](#setmousestate)
- [`loadFont(const std::string&)`](#loadfont)
- [`drawText(const std::string&, Vec2f, const Font&, float, Color, TextStyle)`](#drawtext)
- [`measureText(const std::string&, const Font&, float)`](#measuretext)
- [`charPositions(const std::string&, const Font&, float)`](#charpositions)
- [`createFrameBuffer(Vec2u size)`](#createframebuffer)
- [`resizeFrameBuffer(FrameBuffer& fb, Vec2u newSize)`](#resizeframebuffer)
- [`destroyFrameBuffer(FrameBuffer& fb)`](#destroyframebuffer)
- [`pushFrameBuffer(FrameBuffer&)`](#pushframebuffer)
- [`popFrameBuffer()`](#popframebuffer)
- [`drawFrameBuffer(const FrameBuffer&, Vec2f, Vec2f, Color)`](#drawframebuffer)
- [`clear(Color)`](#clear)
- [`pushScissor(Rectf)`](#pushscissor)
- [`popScissor()`](#popscissor)
- [`pushRoundClip(Rectf, float)`](#pushroundclip)
- [`popRoundClip()`](#poproundclip)
- [`beginGlassSubtree()`](#beginglasssubtree)
- [`endGlassSubtree()`](#endglasssubtree)
- [`setRotation(float, Vec2f)`](#setrotation)
- [`rotate(float)`](#rotate)
- [`clearRotation()`](#clearrotation)
- [`currentViewId()`](#currentviewid)
- [`submitOrtho(uint16_t, Vec2u)`](#submitortho)

---

### Pty

Stubs for the web build, where a page has no shell to attach to -- a browser cannot fork a process, and proxying one to the server would hand every visitor a shell on the host. [Terminal](../elements/widgets/Terminal.hpp.md#terminal) is compiled for the web because it lives under elements/, so it needs these to link; with them it renders as an empty screen and reports that no shell could be started, which is the honest outcome.

> Serving a real shell to a browser would need an explicit, authenticated server-side channel. That is a deliberate decision for whoever deploys the app, not something [UILO](../UILO.hpp.md#uilo) should do implicitly.

---

### SDL_GetMouseState

```cpp
SDL_GetMouseState(float* x, float* y)
```

**Parameters**

- `float* x`
- `float* y`

**Returns** — SDL_MouseButtonFlags

Stub for the shim's declaration of this SDL entry point, which the web build never reaches.

---

### SDL_GetTicks

```cpp
SDL_GetTicks()
```

**Returns** — Uint64

Stub for the shim's declaration of this SDL entry point, which the web build never reaches.

---

### SDL_GetModState

```cpp
SDL_GetModState()
```

**Returns** — SDL_Keymod

Stub for the shim's declaration of this SDL entry point, which the web build never reaches.

---

### SDL_GetKeyboardState

```cpp
SDL_GetKeyboardState(int* numkeys)
```

**Parameters**

- `int* numkeys`

**Returns** — const bool*

A full always-up scancode table. [UILO](../UILO.hpp.md#uilo) scans [0, numkeys) looking for any key held down, and uilo::Keybinds indexes this directly by scancode, so the table has to span the whole range rather than a single entry.

---

### SDL_GetClipboardText

```cpp
SDL_GetClipboardText()
```

**Returns** — char*

Stub for the shim's declaration of this SDL entry point, which the web build never reaches.

---

### SDL_SetClipboardText

```cpp
SDL_SetClipboardText(const char*)
```

**Parameters**

- `const char*`

**Returns** — bool

Stub for the shim's declaration of this SDL entry point, which the web build never reaches.

---

### SDL_free

```cpp
SDL_free(void*)
```

**Parameters**

- `void*`

Stub for the shim's declaration of this SDL entry point, which the web build never reaches.

---

### SDL_StartTextInput

```cpp
SDL_StartTextInput(SDL_Window*)
```

**Parameters**

- `SDL_Window*`

**Returns** — bool

Stub for the shim's declaration of this SDL entry point, which the web build never reaches.

---

### SDL_StopTextInput

```cpp
SDL_StopTextInput(SDL_Window*)
```

**Parameters**

- `SDL_Window*`

**Returns** — bool

Stub for the shim's declaration of this SDL entry point, which the web build never reaches.

---

### SDL_TextInputActive

```cpp
SDL_TextInputActive(SDL_Window*)
```

**Parameters**

- `SDL_Window*`

**Returns** — bool

Stub for the shim's declaration of this SDL entry point, which the web build never reaches.

---

### SDL_AddEventWatch

```cpp
SDL_AddEventWatch(SDL_EventFilter, void*)
```

**Parameters**

- `SDL_EventFilter`
- `void*`

**Returns** — bool

Stub for the shim's declaration of this SDL entry point, which the web build never reaches.

---

### SDL_PollEvent

```cpp
SDL_PollEvent(SDL_Event*)
```

**Parameters**

- `SDL_Event*`

**Returns** — bool

The browser is the input device on this backend, so the queue is always empty and there is no cursor to capture or warp.

---

### SDL_SetWindowRelativeMouseMode

```cpp
SDL_SetWindowRelativeMouseMode(SDL_Window*, bool)
```

**Parameters**

- `SDL_Window*`
- `bool`

**Returns** — bool

Stub for the shim's declaration of this SDL entry point, which the web build never reaches.

---

### SDL_GetWindowRelativeMouseMode

```cpp
SDL_GetWindowRelativeMouseMode(SDL_Window*)
```

**Parameters**

- `SDL_Window*`

**Returns** — bool

Stub for the shim's declaration of this SDL entry point, which the web build never reaches.

---

### SDL_GetRelativeMouseState

```cpp
SDL_GetRelativeMouseState(float* x, float* y)
```

**Parameters**

- `float* x`
- `float* y`

Stub for the shim's declaration of this SDL entry point, which the web build never reaches.

---

### SDL_GetWindowDisplayScale

```cpp
SDL_GetWindowDisplayScale(SDL_Window*)
```

**Parameters**

- `SDL_Window*`

**Returns** — float

Stub for the shim's declaration of this SDL entry point, which the web build never reaches.

---

### SDL_GetError

```cpp
SDL_GetError()
```

**Returns** — const char*

Stub for the shim's declaration of this SDL entry point, which the web build never reaches.

---

### SDL_WasInit

```cpp
SDL_WasInit(SDL_InitFlags)
```

**Parameters**

- `SDL_InitFlags`

**Returns** — SDL_InitFlags

uilo::OS queries. There is no local display on this backend, so every one reports "not initialised / unknown" and OS returns its documented fallbacks (scale 1.0, zero sizes) rather than inventing values.

---

### SDL_GetPrimaryDisplay

```cpp
SDL_GetPrimaryDisplay()
```

**Returns** — SDL_DisplayID

Stub for the shim's declaration of this SDL entry point, which the web build never reaches.

---

### SDL_GetDisplayContentScale

```cpp
SDL_GetDisplayContentScale(SDL_DisplayID)
```

**Parameters**

- `SDL_DisplayID`

**Returns** — float

Stub for the shim's declaration of this SDL entry point, which the web build never reaches.

---

### SDL_GetDisplayBounds

```cpp
SDL_GetDisplayBounds(SDL_DisplayID, SDL_Rect* rect)
```

**Parameters**

- `SDL_DisplayID`
- `SDL_Rect* rect`

**Returns** — bool

Stub for the shim's declaration of this SDL entry point, which the web build never reaches.

---

### SDL_GetCurrentDisplayMode

```cpp
SDL_GetCurrentDisplayMode(SDL_DisplayID)
```

**Parameters**

- `SDL_DisplayID`

**Returns** — const SDL_DisplayMode*

Stub for the shim's declaration of this SDL entry point, which the web build never reaches.

---

### SDL_GetDisplays

```cpp
SDL_GetDisplays(int* count)
```

**Parameters**

- `int* count`

**Returns** — SDL_DisplayID*

Stub for the shim's declaration of this SDL entry point, which the web build never reaches.

---

### SDL_GetSystemTheme

```cpp
SDL_GetSystemTheme()
```

**Returns** — SDL_SystemTheme

Stub for the shim's declaration of this SDL entry point, which the web build never reaches.

---

### SDL_GetNumLogicalCPUCores

```cpp
SDL_GetNumLogicalCPUCores()
```

**Returns** — int

Stub for the shim's declaration of this SDL entry point, which the web build never reaches.

---

### SDL_GetSystemRAM

```cpp
SDL_GetSystemRAM()
```

**Returns** — int

Stub for the shim's declaration of this SDL entry point, which the web build never reaches.

---

### SDL_GetBasePath

```cpp
SDL_GetBasePath()
```

**Returns** — const char*

Stub for the shim's declaration of this SDL entry point, which the web build never reaches.

---

### SDL_GetPrefPath

```cpp
SDL_GetPrefPath(const char*, const char*)
```

**Parameters**

- `const char*`
- `const char*`

**Returns** — char*

Stub for the shim's declaration of this SDL entry point, which the web build never reaches.

---

### SDL_GetWindowSize

```cpp
SDL_GetWindowSize(SDL_Window*, int* w, int* h)
```

**Parameters**

- `SDL_Window*`
- `int* w`
- `int* h`

**Returns** — bool

Stub for the shim's declaration of this SDL entry point, which the web build never reaches.

---

### SDL_GetWindowSizeInPixels

```cpp
SDL_GetWindowSizeInPixels(SDL_Window*, int* w, int* h)
```

**Parameters**

- `SDL_Window*`
- `int* w`
- `int* h`

**Returns** — bool

Stub for the shim's declaration of this SDL entry point, which the web build never reaches.

---

### SDL_GetWindowProperties

```cpp
SDL_GetWindowProperties(SDL_Window*)
```

**Parameters**

- `SDL_Window*`

**Returns** — SDL_PropertiesID

Stub for the shim's declaration of this SDL entry point, which the web build never reaches.

---

### SDL_GetPointerProperty

```cpp
SDL_GetPointerProperty(SDL_PropertiesID, const char*, void* default_value)
```

**Parameters**

- `SDL_PropertiesID`
- `const char*`
- `void* default_value`

**Returns** — void*

Stub for the shim's declaration of this SDL entry point, which the web build never reaches.

---

### configureMacWindowForLiveResize

```cpp
configureMacWindowForLiveResize(void*)
```

**Parameters**

- `void*`

**Returns** — bool

Stub for the macOS window shim; a headless build has no window.

---

### installMacScrollMonitor

```cpp
installMacScrollMonitor(std::function<bool(float, float, bool)>)
```

**Parameters**

- `std::function<bool(float`
- `float`
- `bool)>`

**Returns** — bool

Stub for the macOS input shim, which a headless build never installs.

---

### installMacZoomMonitor

```cpp
installMacZoomMonitor(std::function<bool(float)>)
```

**Parameters**

- `std::function<bool(float)>`

**Returns** — bool

Stub for the macOS input shim, which a headless build never installs.

---

### tickMacScrollMomentum

```cpp
tickMacScrollMomentum(float)
```

**Parameters**

- `float`

Stub for the macOS input shim, which a headless build never installs.

---

### cancelMacScrollMomentum

```cpp
cancelMacScrollMomentum()
```

Stub for the macOS input shim, which a headless build never installs.

---

### Renderer

```cpp
Renderer()
```

**Returns** — R

No-op in the headless build, which never draws.

---

### ~Renderer

```cpp
~Renderer()
```

**Returns** — R

No-op in the headless build, which never draws.

---

### init

```cpp
init(uint32_t width, uint32_t height, const std::string&, uint8_t msaa)
```

**Parameters**

- `uint32_t width`
- `uint32_t height`
- `const std::string&`
- `uint8_t msaa`

**Returns** — bool

No-op in the headless build, which never draws.

---

### attach

```cpp
attach(SDL_Window*, uint16_t)
```

**Parameters**

- `SDL_Window*`
- `uint16_t`

**Returns** — bool

No-op in the headless build, which never draws.

---

### shutdown

```cpp
shutdown()
```

No-op in the headless build, which never draws.

---

### beginFrame

```cpp
beginFrame()
```

No-op in the headless build, which never draws.

---

### endFrame

```cpp
endFrame()
```

No-op in the headless build, which never draws.

---

### getSize

```cpp
getSize()
```

**Returns** — [Vec2u](../utils/Math.hpp.md#vec2u)

No-op in the headless build, which never draws.

---

### setTitle

```cpp
setTitle(const std::string&)
```

**Parameters**

- `const std::string&`

No-op in the headless build, which never draws.

---

### setVsync

```cpp
setVsync(bool)
```

**Parameters**

- `bool`

No-op in the headless build, which never draws.

---

### getVsync

```cpp
getVsync()
```

**Returns** — bool

No-op in the headless build, which never draws.

---

### setFramerateLimit

```cpp
setFramerateLimit(float)
```

**Parameters**

- `float`

No-op in the headless build, which never draws.

---

### getFramerateLimit

```cpp
getFramerateLimit()
```

**Returns** — float

No-op in the headless build, which never draws.

---

### getStats

```cpp
getStats()
```

**Returns** — RendererStats

No-op in the headless build, which never draws.

---

### setCursor

```cpp
setCursor(CursorType)
```

**Parameters**

- `CursorType`

No-op in the headless build, which never draws.

---

### draw

```cpp
draw(const Rect&)
```

**Parameters**

- `const Rect&`

No-op in the headless build, which never draws.

---

### draw

```cpp
draw(const RoundedRect&)
```

**Parameters**

- `const RoundedRect&`

No-op in the headless build, which never draws.

---

### draw

```cpp
draw(const Circle&)
```

**Parameters**

- `const Circle&`

No-op in the headless build, which never draws.

---

### draw

```cpp
draw(const Triangle&)
```

**Parameters**

- `const Triangle&`

No-op in the headless build, which never draws.

---

### draw

```cpp
draw(const Line&)
```

**Parameters**

- `const Line&`

No-op in the headless build, which never draws.

---

### drawLines

```cpp
drawLines(const Line*, size_t)
```

**Parameters**

- `const Line*`
- `size_t`

No-op in the headless build, which never draws.

---

### drawArc

```cpp
drawArc(Vec2f, float, float, float, float, Color, int)
```

**Parameters**

- `Vec2f`
- `float`
- `float`
- `float`
- `float`
- `Color`
- `int`

No-op in the headless build, which never draws.

---

### loadTexture

```cpp
loadTexture(const std::string&)
```

**Parameters**

- `const std::string&`

**Returns** — Texture

No-op in the headless build, which never draws.

---

### destroyTexture

```cpp
destroyTexture(Texture& tex)
```

**Parameters**

- `Texture& tex`

No-op in the headless build, which never draws.

---

### loadImagePixels

```cpp
loadImagePixels(const std::string&, std::vector<uint8_t>&, uint32_t&, uint32_t&)
```

**Parameters**

- `const std::string&`
- `std::vector<uint8_t>&`
- `uint32_t&`
- `uint32_t&`

**Returns** — bool

No-op in the headless build, which never draws.

---

### createTexture

```cpp
createTexture(uint16_t, uint16_t)
```

**Parameters**

- `uint16_t`
- `uint16_t`

**Returns** — Texture

No-op in the headless build, which never draws.

---

### updateTexture

```cpp
updateTexture(const Texture&, const uint8_t*)
```

**Parameters**

- `const Texture&`
- `const uint8_t*`

No-op in the headless build, which never draws.

---

### drawImage

```cpp
drawImage(const Rectf&, const Texture&, Color, Rectf, bool, bool, bool)
```

**Parameters**

- `const Rectf&`
- `const Texture&`
- `Color`
- `Rectf`
- `bool`
- `bool`
- `bool`

No-op in the headless build, which never draws.

---

### drawGlass

```cpp
drawGlass(const Rectf&, const Material&, Color)
```

**Parameters**

- `const Rectf&`
- `const Material&`
- `Color`

No-op in the headless build, which never draws.

---

### setMouseState

```cpp
setMouseState(Vec2f pos)
```

**Parameters**

- `Vec2f pos`

No-op in the headless build, which never draws.

---

### loadFont

```cpp
loadFont(const std::string&)
```

**Parameters**

- `const std::string&`

**Returns** — [Font](../renderer/Renderer.hpp.md#font)

No-op in the headless build, which never draws.

---

### drawText

```cpp
drawText(const std::string&, Vec2f, const Font&, float, Color, TextStyle)
```

**Parameters**

- `const std::string&`
- `Vec2f`
- `const Font&`
- `float`
- `Color`
- `TextStyle`

No-op in the headless build, which never draws.

---

### measureText

```cpp
measureText(const std::string&, const Font&, float)
```

**Parameters**

- `const std::string&`
- `const Font&`
- `float`

**Returns** — [TextMetrics](../renderer/Renderer.hpp.md#textmetrics)

No-op in the headless build, which never draws.

---

### charPositions

```cpp
charPositions(const std::string&, const Font&, float)
```

**Parameters**

- `const std::string&`
- `const Font&`
- `float`

**Returns** — std::vector&lt;[Vec2f](../utils/Math.hpp.md#vec2f)&gt;

No-op in the headless build, which never draws.

---

### createFrameBuffer

```cpp
createFrameBuffer(Vec2u size)
```

**Parameters**

- `Vec2u size`

**Returns** — FrameBuffer

No-op in the headless build, which never draws.

---

### resizeFrameBuffer

```cpp
resizeFrameBuffer(FrameBuffer& fb, Vec2u newSize)
```

**Parameters**

- `FrameBuffer& fb`
- `Vec2u newSize`

No-op in the headless build, which never draws.

---

### destroyFrameBuffer

```cpp
destroyFrameBuffer(FrameBuffer& fb)
```

**Parameters**

- `FrameBuffer& fb`

No-op in the headless build, which never draws.

---

### pushFrameBuffer

```cpp
pushFrameBuffer(FrameBuffer&)
```

**Parameters**

- `FrameBuffer&`

No-op in the headless build, which never draws.

---

### popFrameBuffer

```cpp
popFrameBuffer()
```

No-op in the headless build, which never draws.

---

### drawFrameBuffer

```cpp
drawFrameBuffer(const FrameBuffer&, Vec2f, Vec2f, Color)
```

**Parameters**

- `const FrameBuffer&`
- `Vec2f`
- `Vec2f`
- `Color`

No-op in the headless build, which never draws.

---

### clear

```cpp
clear(Color)
```

**Parameters**

- `Color`

No-op in the headless build, which never draws.

---

### pushScissor

```cpp
pushScissor(Rectf)
```

**Parameters**

- `Rectf`

No-op in the headless build, which never draws.

---

### popScissor

```cpp
popScissor()
```

No-op in the headless build, which never draws.

---

### pushRoundClip

```cpp
pushRoundClip(Rectf, float)
```

**Parameters**

- `Rectf`
- `float`

No-op in the headless build, which never draws.

---

### popRoundClip

```cpp
popRoundClip()
```

No-op in the headless build, which never draws.

---

### beginGlassSubtree

```cpp
beginGlassSubtree()
```

No-op in the headless build, which never draws.

---

### endGlassSubtree

```cpp
endGlassSubtree()
```

No-op in the headless build, which never draws.

---

### setRotation

```cpp
setRotation(float, Vec2f)
```

**Parameters**

- `float`
- `Vec2f`

No-op in the headless build, which never draws.

---

### rotate

```cpp
rotate(float)
```

**Parameters**

- `float`

No-op in the headless build, which never draws.

---

### clearRotation

```cpp
clearRotation()
```

No-op in the headless build, which never draws.

---

### currentViewId

```cpp
currentViewId()
```

**Returns** — uint16_t

No-op in the headless build, which never draws.

---

### submitOrtho

```cpp
submitOrtho(uint16_t, Vec2u)
```

**Parameters**

- `uint16_t`
- `Vec2u`

No-op in the headless build, which never draws.
