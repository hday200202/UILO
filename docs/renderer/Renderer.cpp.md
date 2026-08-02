# Renderer.cpp

`include/renderer/Renderer.cpp`

[← index](../README.md)

## Functions

- [`ensureLayouts()`](#ensurelayouts)
- [`initShaders()`](#initshaders)
- [`shutdownResources()`](#shutdownresources)
- [`destroySceneFramebuffers()`](#destroysceneframebuffers)
- [`ensureSceneFramebuffers(uint32_t width, uint32_t height)`](#ensuresceneframebuffers)
- [`runBlurPasses(uint32_t width, uint32_t height)`](#runblurpasses)
- [`compositeSceneToBackbuffer(...)`](#compositescenetobackbuffer)
- [`Renderer() : m_impl(std::make_unique&lt;Impl&gt;())`](#renderer)
- [`~Renderer() { shutdown()`](#renderer)
- [`init(uint32_t width, uint32_t height, const std::string& title, uint8_t msaa)`](#init)
- [`attach(SDL_Window* hostWindow, uint16_t baseView)`](#attach)
- [`shutdown()`](#shutdown)
- [`getSize()`](#getsize)
- [`setTitle(const std::string& title)`](#settitle)
- [`getStats()`](#getstats)
- [`setVsync(bool enabled)`](#setvsync)
- [`getVsync()`](#getvsync)
- [`setFramerateLimit(float fps)`](#setframeratelimit)
- [`getFramerateLimit()`](#getframeratelimit)
- [`setCursor(CursorType type)`](#setcursor)
- [`beginFrame()`](#beginframe)
- [`endFrame()`](#endframe)
- [`submitOrtho(uint16_t viewId, Vec2u size)`](#submitortho)
- [`currentViewId()`](#currentviewid)
- [`createFrameBuffer(Vec2u size)`](#createframebuffer)
- [`resizeFrameBuffer(FrameBuffer& fb, Vec2u newSize)`](#resizeframebuffer)
- [`destroyFrameBuffer(FrameBuffer& fb)`](#destroyframebuffer)
- [`pushFrameBuffer(FrameBuffer& fb)`](#pushframebuffer)
- [`popFrameBuffer()`](#popframebuffer)
- [`beginGlassSubtree()`](#beginglasssubtree)
- [`endGlassSubtree()`](#endglasssubtree)
- [`drawFrameBuffer(const FrameBuffer& fb, Vec2f dest, Vec2f size, Color tint)`](#drawframebuffer)
- [`pushScissor(Rectf b)`](#pushscissor)
- [`popScissor()`](#popscissor)
- [`pushRoundClip(Rectf b, float radius)`](#pushroundclip)
- [`popRoundClip()`](#poproundclip)
- [`setRotation(float degrees, Vec2f pivot)`](#setrotation)
- [`rotate(float deltaDegrees)`](#rotate)
- [`clearRotation()`](#clearrotation)
- [`setMouseState(Vec2f mousePosFbPx)`](#setmousestate)
- [`clear(Color color)`](#clear)
- [`refreshClipCache()`](#refreshclipcache)
- [`batchStateMatches(uint16_t viewId)`](#batchstatematches)
- [`captureBatchState(uint16_t viewId)`](#capturebatchstate)
- [`packAvgColor(Color a, Color b, Color c, Color d)`](#packavgcolor)
- [`draw(const Rect& r)`](#draw)
- [`flushSolidBatch()`](#flushsolidbatch)
- [`draw(const RoundedRect& rr)`](#draw)
- [`draw(const Circle& c)`](#draw)
- [`draw(const Triangle& t)`](#draw)
- [`draw(const Line& l)`](#draw)
- [`drawLines(const Line* lines, size_t count)`](#drawlines)
- [`drawArc(...)`](#drawarc)

---

### ensureLayouts

```cpp
ensureLayouts()
```

============================================================ ================ Impl ====================================== ======================================

---

### initShaders

```cpp
initShaders()
```

**Returns** — bool

Compiles the embedded shader programs and creates the uniform handles the renderer draws through. Shaders are embedded rather than loaded, so a built application needs no shader files beside it.

---

### shutdownResources

```cpp
shutdownResources()
```

Destroys every GPU resource the renderer created -- programs, uniforms, framebuffers, textures and font atlases -- in an order that leaves nothing referencing a destroyed handle.

---

### destroySceneFramebuffers

```cpp
destroySceneFramebuffers()
```

============================================================ ================ Offscreen scene + blur ladder ([Material](../utils/Material.hpp.md#material)::Glass) ========================================== ==================================

---

### ensureSceneFramebuffers

```cpp
ensureSceneFramebuffers(uint32_t width, uint32_t height)
```

**Parameters**

- `uint32_t width`
- `uint32_t height`

Creates or resizes the offscreen targets the scene is composited through. Recreated only when the size actually changes, since reallocating a framebuffer every frame would stall the GPU.

---

### runBlurPasses

```cpp
runBlurPasses(uint32_t width, uint32_t height)
```

**Parameters**

- `uint32_t width`
- `uint32_t height`

Runs the separable blur used by glass materials: a horizontal pass then a vertical one, at reduced resolution. Separating the two turns an NxN kernel into two N-wide ones, which is what makes a wide blur affordable.

---

### compositeSceneToBackbuffer

```cpp
compositeSceneToBackbuffer(...)
```

**Parameters**

- `uint32_t width`
- `uint32_t height`
- `const bgfx::VertexLayout& layout`
- `bgfx::ProgramHandle program`

Draws the finished offscreen scene onto the backbuffer as a fullscreen quad, which is the last step of a frame.

---

### Renderer

```cpp
Renderer() : m_impl(std::make_unique<Impl>())
```

**Parameters**

- `) : m_impl(std::make_unique<Impl>()`

**Returns** — R

============================================================ ================ [Renderer](RendererImpl.hpp.md#renderer) ctor/dtor/lifecycle ============== ============================================================ ==

---

### ~Renderer

```cpp
~Renderer() { shutdown()
```

**Parameters**

- `) { shutdown(`

**Returns** — R

Shuts the renderer down, releasing the GPU resources and, unless attached to a host's context, the window and bgfx too.

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

Creates the window and brings bgfx up on it, then builds the shaders and the initial framebuffers. Reports false rather than throwing if any stage fails, so an application can fall back or exit cleanly.

---

### attach

```cpp
attach(SDL_Window* hostWindow, uint16_t baseView)
```

**Parameters**

- `SDL_Window* hostWindow`
- `uint16_t baseView`

**Returns** — bool

Embedded mode: binds to a window and bgfx context a host already owns. Skips creation entirely, rebases [UILO](../UILO.hpp.md#uilo)'s views to start after the host's, and clears transparent so the UI composites over the host's image rather than erasing it.

---

### shutdown

```cpp
shutdown()
```

Releases everything init() created, and the window and bgfx context too unless this renderer was attached to a host's.

---

### getSize

```cpp
getSize()
```

**Returns** — [Vec2u](../utils/Math.hpp.md#vec2u)

============================================================ ================ Window / cursor =========================== =================================================

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

Draw-call and vertex counters for the previous frame, for a HUD or profiling.

---

### setVsync

```cpp
setVsync(bool enabled)
```

**Parameters**

- `bool enabled`

Turns vertical sync on or off, which takes effect on the next reset.

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

Applies a cursor shape to the window, creating and caching the system cursor on first use.

---

### beginFrame

```cpp
beginFrame()
```

============================================================ ================ Frame lifecycle / projection ============== ============================================================ ==

---

### endFrame

```cpp
endFrame()
```

Flush any rects still sitting in the solid-rect batch from the last user draw call before kicking off internal passes.

---

### submitOrtho

```cpp
submitOrtho(uint16_t viewId, Vec2u size)
```

**Parameters**

- `uint16_t viewId`
- `Vec2u size`

Sets a view's orthographic projection so one unit is one pixel with the origin at the top left, which is the space every element's bounds are expressed in.

---

### currentViewId

```cpp
currentViewId()
```

**Returns** — uint16_t

The view currently being drawn into, which is the top of the framebuffer stack or the scene view when that stack is empty.

---

### createFrameBuffer

```cpp
createFrameBuffer(Vec2u size)
```

**Parameters**

- `Vec2u size`

**Returns** — FrameBuffer

============================================================ ================ Framebuffer =============================== =============================================

---

### resizeFrameBuffer

```cpp
resizeFrameBuffer(FrameBuffer& fb, Vec2u newSize)
```

**Parameters**

- `FrameBuffer& fb`
- `Vec2u newSize`

Resizes a framebuffer by recreating it, since a bgfx framebuffer's dimensions are fixed at creation.

---

### destroyFrameBuffer

```cpp
destroyFrameBuffer(FrameBuffer& fb)
```

**Parameters**

- `FrameBuffer& fb`

Destroys a framebuffer and its texture.

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

### beginGlassSubtree

```cpp
beginGlassSubtree()
```

Opens a glass group, so every element drawn until the matching end composites into the blur the material established rather than each blurring the backdrop separately.

---

### endGlassSubtree

```cpp
endGlassSubtree()
```

Closes the glass group opened by beginGlassSubtree.

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

Draws a framebuffer's texture as a tinted quad, which is how an offscreen subtree is composited back into the scene.

---

### pushScissor

```cpp
pushScissor(Rectf b)
```

**Parameters**

- `Rectf b`

============================================================ ================ Scissor =================================== =========================================

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

No flush: rotation is applied CPU-side when vertices are appended, so rects already queued in the batch keep the rotation they were emitted under.

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

### setMouseState

```cpp
setMouseState(Vec2f mousePosFbPx)
```

**Parameters**

- `Vec2f mousePosFbPx`

Detect motion vs the previous frame so animated materials can fade the ripple amplitude when the cursor sits still. Threshold avoids sub-pixel noise from triggering constant "moving" state.

---

### clear

```cpp
clear(Color color)
```

**Parameters**

- `Color color`

Clears the scene target to a colour, which begins a frame.

---

### refreshClipCache

```cpp
refreshClipCache()
```

Walk the clip stack from the top down and collect the two top- most rounded entries (radius > 0). The first becomes the "inner" SDF (the shape being drawn, e.g. a button's own rounded body); the second becomes the "outer" SDF (the enclosing rounded ancestor, e.g. the parent panel). Both are applied in the fragment shader, so a child element stays its own shape AND gets cropped by the parent's rounded corners.

---

### batchStateMatches

```cpp
batchStateMatches(uint16_t viewId)
```

**Parameters**

- `uint16_t viewId`

**Returns** — bool

Whether a new shape can join the batch already being built. Batching is only valid while the view, clip and transform are unchanged, so this is what decides between appending and flushing first.

---

### captureBatchState

```cpp
captureBatchState(uint16_t viewId)
```

**Parameters**

- `uint16_t viewId`

Records the view, clip and transform a batch was started under, so batchStateMatches has something to compare against.

---

### packAvgColor

```cpp
packAvgColor(Color a, Color b, Color c, Color d)
```

**Parameters**

- `Color a`
- `Color b`
- `Color c`
- `Color d`

**Returns** — inline uint32_t

Center-vertex color for gradient quads: the average of the four corners.

---

### draw

```cpp
draw(const Rect& r)
```

**Parameters**

- `const Rect& r`

Queues an axis-aligned rectangle, with an optional inside border and per-corner gradient, into the solid batch.

---

### flushSolidBatch

```cpp
flushSolidBatch()
```

Submits the accumulated solid geometry as one draw call and empties the batch. Called whenever the state changes or a textured draw has to be ordered against it.

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

Queues a circle, tessellated to a segment count that keeps the silhouette smooth at its drawn size.

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

Queues a single line as a quad expanded to its thickness.

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
