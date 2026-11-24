# Renderer.cpp

`include/renderer/Renderer.cpp`

[← index](../README.md)

## Functions

- [`toValoColor(Color c)`](#tovalocolor)
- [`sleepUntilNextFrame(double interval, uint64_t& nextTick)`](#sleepuntilnextframe)
- [`createResources()`](#createresources)
- [`shutdownResources()`](#shutdownresources)
- [`applyLayerOrder()`](#applylayerorder)
- [`refreshClipCache()`](#refreshclipcache)
- [`beginShape(uint32_t mode, valo::TextureID tex, const float imgFlags[4])`](#beginshape)
- [`flushBatch()`](#flushbatch)
- [`Renderer()`](#renderer)
- [`~Renderer()`](#renderer)
- [`init(uint32_t width, uint32_t height, const std::string& title, uint8_t msaa)`](#init)
- [`shutdown()`](#shutdown)
- [`getSize()`](#getsize)
- [`window()`](#window)
- [`setTitle(const std::string& title)`](#settitle)
- [`getStats()`](#getstats)
- [`setVsync(bool enabled)`](#setvsync)
- [`getVsync()`](#getvsync)
- [`setFramerateLimit(float fps)`](#setframeratelimit)
- [`getFramerateLimit()`](#getframeratelimit)
- [`setCursor(CursorType type)`](#setcursor)
- [`beginFrame()`](#beginframe)
- [`endFrame()`](#endframe)
- [`createFrameBuffer(Vec2u size)`](#createframebuffer)
- [`resizeFrameBuffer(FrameBuffer& fb, Vec2u newSize)`](#resizeframebuffer)
- [`destroyFrameBuffer(FrameBuffer& fb)`](#destroyframebuffer)
- [`pushFrameBuffer(FrameBuffer& fb)`](#pushframebuffer)
- [`popFrameBuffer()`](#popframebuffer)
- [`drawFrameBuffer(const FrameBuffer& fb, Vec2f dest, Vec2f size, Color tint)`](#drawframebuffer)
- [`pushScissor(Rectf b)`](#pushscissor)
- [`popScissor()`](#popscissor)
- [`pushRoundClip(Rectf b, float radius)`](#pushroundclip)
- [`popRoundClip()`](#poproundclip)
- [`setRotation(float degrees, Vec2f pivot)`](#setrotation)
- [`rotate(float deltaDegrees)`](#rotate)
- [`clearRotation()`](#clearrotation)
- [`clear(Color color)`](#clear)
- [`avgColor(Color a, Color b, Color c, Color d)`](#avgcolor)
- [`draw(const Rect& r)`](#draw)
- [`draw(const RoundedRect& rr)`](#draw)
- [`draw(const Circle& c)`](#draw)
- [`draw(const Triangle& t)`](#draw)
- [`draw(const Line& l)`](#draw)
- [`drawLines(const Line* lines, size_t count)`](#drawlines)
- [`drawArc(...)`](#drawarc)

---

### toValoColor

```cpp
toValoColor(Color c)
```

**Parameters**

- `Color c`

**Returns** — valo::[Color](../utils/Color.hpp.md#color)

A [UILO](../UILO.hpp.md#uilo) colour as VALO's 0..1 floats, for layer clears.

---

### sleepUntilNextFrame

```cpp
sleepUntilNextFrame(double interval, uint64_t& nextTick)
```

**Parameters**

- `double interval -- seconds per frame; uint64_t& nextTick -- steady_clock ns of the next deadline`
- `0 to re-anchor`

The frame-rate cap. Sleeps most of the remaining time and spins the last half millisecond, so scheduler latency does not overshoot the deadline; re-anchors when the loop has fallen more than a whole frame behind rather than trying to catch up.

---

### createResources

```cpp
createResources()
```

**Returns** — bool

Creates what every frame draws through: the UI program and the arena's vertex and index buffers. The buffers start with one placeholder element because VALO creates a buffer from data; the first frame replaces it.

---

### shutdownResources

```cpp
shutdownResources()
```

Releases every GPU resource the renderer created -- textures, font atlases, framebuffer layers, the program and the arena -- then VALO itself and the window.

---

### applyLayerOrder

```cpp
applyLayerOrder()
```

Framebuffer layers run before the window's, so a framebuffer drawn into the scene has already been rendered that frame.

---

### refreshClipCache

```cpp
refreshClipCache()
```

Walk the clip stack from the top down and collect the two top- most rounded entries (radius > 0). The first becomes the "inner" SDF (the shape being drawn, e.g. a button's own rounded body); the second becomes the "outer" SDF (the enclosing rounded ancestor, e.g. the parent panel). Both are applied in the fragment shader, so a child element stays its own shape AND gets cropped by the parent's rounded corners.

---

### beginShape

```cpp
beginShape(uint32_t mode, valo::TextureID tex, const float imgFlags[4])
```

**Parameters**

- `uint32_t mode`
- `valo::TextureID tex`
- `const float imgFlags[4]`

**Returns** — bool -- false when the current scissor is empty

Makes sure the batch about to receive a shape's geometry was opened under exactly this shape's draw state -- target, shader mode, texture, image flags, both rounded clips and the scissor. Anything different submits the batch so far and starts a new one, so draw order is always submission order.

---

### flushBatch

```cpp
flushBatch()
```

Submits the open batch as one VALO command and closes it. The command names a range of the arena, which is uploaded at endFrame before VALO runs the frame's queue.

---

### Renderer

```cpp
Renderer()
```

**Returns** — Renderer

Creates an uninitialised renderer; init() opens the window.

---

### ~Renderer

```cpp
~Renderer()
```

**Returns** — n/a

Shuts the renderer down, releasing the GPU resources, VALO and the window.

---

### init

```cpp
init(uint32_t width, uint32_t height, const std::string& title, uint8_t msaa)
```

**Parameters**

- `uint32_t width`
- `uint32_t height`
- `const std::string& title`
- `uint8_t msaa`

**Returns** — bool

Opens the window and brings VALO up on it, then builds the program and the arena. Vulkan is tried first where VALO was built with it, and the software rasteriser is the fallback, so a machine without a usable GPU still opens. UILO_RENDERER=vulkan | software | auto forces the choice. Reports false rather than throwing if every option fails.

---

### shutdown

```cpp
shutdown()
```

Releases everything init() created: GPU resources, VALO and the window.

---

### getSize

```cpp
getSize()
```

**Returns** — [Vec2u](../utils/Math.hpp.md#vec2u)

The window's size in pixels -- the space every element's bounds are expressed in.

---

### window

```cpp
window()
```

**Returns** — valo::Window*

The window [UILO](../UILO.hpp.md#uilo) reads input from, null before init().

---

### setTitle

```cpp
setTitle(const std::string& title)
```

**Parameters**

- `const std::string& title`

Sets the window title.

---

### getStats

```cpp
getStats()
```

**Returns** — RendererStats

Draw-command and vertex counters for the previous frame, and how long VALO took to run it, for a HUD or profiling.

---

### setVsync

```cpp
setVsync(bool enabled)
```

**Parameters**

- `bool enabled`

Turns vertical sync on or off.

---

### getVsync

```cpp
getVsync()
```

**Returns** — bool

Whether vertical sync is on.

---

### setFramerateLimit

```cpp
setFramerateLimit(float fps)
```

**Parameters**

- `float fps`

Caps the frame rate by sleeping out the remainder of each frame. 0 removes the cap. Useful with vsync off, where an unbounded loop would spin the GPU for no visible benefit.

---

### getFramerateLimit

```cpp
getFramerateLimit()
```

**Returns** — float

The current frame-rate cap, 0 when uncapped.

---

### setCursor

```cpp
setCursor(CursorType type)
```

**Parameters**

- `CursorType type`

Applies a cursor shape to the window. VALO caches the system cursors.

---

### beginFrame

```cpp
beginFrame()
```

Starts a frame: picks up a window resize, and resets the arena, the batch, the clip and target stacks and any rotation left set by the previous frame. Nothing reaches VALO until endFrame.

---

### endFrame

```cpp
endFrame()
```

Submits the last batch, uploads the arena and any glyphs packed this frame, and hands the frame to VALO: the clear colour the frame ended up with is applied first, then every command in submission order. A minimised window renders nothing.

---

### createFrameBuffer

```cpp
createFrameBuffer(Vec2u size)
```

**Parameters**

- `Vec2u size`

**Returns** — FrameBuffer

A render target with its own layer. Its layer runs before the window's, and it clears to transparent at the start of every frame.

---

### resizeFrameBuffer

```cpp
resizeFrameBuffer(FrameBuffer& fb, Vec2u newSize)
```

**Parameters**

- `FrameBuffer& fb`
- `Vec2u newSize`

Resizes a framebuffer by recreating it.

---

### destroyFrameBuffer

```cpp
destroyFrameBuffer(FrameBuffer& fb)
```

**Parameters**

- `FrameBuffer& fb`

Destroys a framebuffer's target and retires its layer.

---

### pushFrameBuffer

```cpp
pushFrameBuffer(FrameBuffer& fb)
```

**Parameters**

- `FrameBuffer& fb`

Redirects drawing into a framebuffer until the matching pop. Used to render a subtree offscreen so it can be composited as a unit.

---

### popFrameBuffer

```cpp
popFrameBuffer()
```

Returns drawing to whatever target was active before the matching push.

---

### drawFrameBuffer

```cpp
drawFrameBuffer(const FrameBuffer& fb, Vec2f dest, Vec2f size, Color tint)
```

**Parameters**

- `const FrameBuffer& fb`
- `Vec2f dest`
- `Vec2f size`
- `Color tint`

Draws a framebuffer's contents as a tinted quad, which is how an offscreen subtree is composited back into the scene.

---

### pushScissor

```cpp
pushScissor(Rectf b)
```

**Parameters**

- `Rectf b`

Clips subsequent drawing to a rectangle, intersected with the clip already in force. Draws pick the top of the stack up as their command's scissor.

---

### popScissor

```cpp
popScissor()
```

Restores the clip rectangle in force before the matching push.

---

### pushRoundClip

```cpp
pushRoundClip(Rectf b, float radius)
```

**Parameters**

- `Rectf b`
- `float radius`

Clips to a rounded rectangle. A radius of 0 is a plain scissor; anything larger also feeds the rounded mask the shaders test against, so content cannot spill past a corner.

---

### popRoundClip

```cpp
popRoundClip()
```

Restores the rounded clip in force before the matching push.

---

### setRotation

```cpp
setRotation(float degrees, Vec2f pivot)
```

**Parameters**

- `float degrees`
- `Vec2f pivot`

Rotates subsequent drawing about a pivot. Applied CPU-side as vertices are emitted, so geometry already in the arena keeps the rotation it was emitted under.

---

### rotate

```cpp
rotate(float deltaDegrees)
```

**Parameters**

- `float deltaDegrees`

Rotates subsequent drawing about the current origin, accumulating with any rotation already in force.

---

### clearRotation

```cpp
clearRotation()
```

Drops any accumulated rotation.

---

### clear

```cpp
clear(Color color)
```

**Parameters**

- `Color color`

Sets the colour the current target is cleared to at the start of the frame. The last call in a frame wins and is applied before any of the frame's draws, whenever it was made.

---

### avgColor

```cpp
avgColor(Color a, Color b, Color c, Color d)
```

**Parameters**

- `Color a`
- `Color b`
- `Color c`
- `Color d`

**Returns** — [Color](../utils/Color.hpp.md#color)

Center-vertex color for gradient quads: the average of the four corners.

---

### draw

```cpp
draw(const Rect& r)
```

**Parameters**

- `const Rect& r`

Queues an axis-aligned rectangle, with an optional inside border and per-corner gradient, into the batch.

---

### draw

```cpp
draw(const RoundedRect& rr)
```

**Parameters**

- `const RoundedRect& rr`

Queues a rounded rectangle. The fill is masked by a signed- distance test in the shader; the border is a true inside ring -- four annular corners and four straight bars -- drawn after the fill so it is visible over a transparent one.

---

### draw

```cpp
draw(const Circle& c)
```

**Parameters**

- `const Circle& c`

Queues a circle as a rounded rect with radius == half-size, so the fragment shader's SDF gives it sub-pixel antialiasing.

---

### draw

```cpp
draw(const Triangle& t)
```

**Parameters**

- `const Triangle& t`

Queues a triangle.

---

### draw

```cpp
draw(const Line& l)
```

**Parameters**

- `const Line& l`

Queues a single line as a quad expanded to its thickness, with a one-pixel alpha skirt either side for antialiasing.

---

### drawLines

```cpp
drawLines(const Line* lines, size_t count)
```

**Parameters**

- `const Line* lines`
- `size_t count`

Draws many lines in a single batch, which is what keeps a subdivision grid or a waveform to one draw call instead of hundreds.

---

### drawArc

```cpp
drawArc(...)
```

**Parameters**

- `Vec2f center`
- `float innerR`
- `float outerR`
- `float startDeg`
- `float endDeg`
- `Color color`
- `int segments`

Draws an annular sector between two radii, antialiased by fading out over about a pixel past each edge, angular and radial alike. A zero inner radius makes it a filled wedge instead, in which case the inner fade is dropped.
