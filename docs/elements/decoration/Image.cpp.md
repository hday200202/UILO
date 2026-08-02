# Image.cpp

`include/elements/decoration/Image.cpp`

[← index](../../README.md)

## Functions

- [`Image(Modifier modifier, ImageOptions options, const std::string& name)`](#image)
- [`~Image()`](#image)
- [`init()`](#init)
- [`rebuildTexture()`](#rebuildtexture)
- [`isLoaded()`](#isloaded)
- [`releaseOwnedTexture()`](#releaseownedtexture)
- [`ensurePixels()`](#ensurepixels)
- [`getPixel(const uint32_t x, const uint32_t y)`](#getpixel)
- [`setPixel(const uint32_t x, const uint32_t y, const Color& color)`](#setpixel)
- [`syncPixels()`](#syncpixels)
- [`update(Rectf& parentBounds, float dt)`](#update)
- [`render()`](#render)

---

### Image

```cpp
Image(Modifier modifier, ImageOptions options, const std::string& name)
```

**Parameters**

- `Modifier modifier`
- `ImageOptions options`
- `const std::string& name`

**Returns** — [Image](Image.hpp.md#image)

Constructs an image element from a modifier and its options. The texture is not loaded here: there is no renderer until the element is bound to a [UILO](../../UILO.hpp.md#uilo), so that waits for the first update.

---

### ~Image

```cpp
~Image()
```

Releases the private texture a pixel write may have created. A path-cached texture belongs to the [Renderer](../../renderer/RendererImpl.hpp.md#renderer) and is left alone.

---

### init

```cpp
init()
```

**Returns** — void

Loads the texture, once, on the first update that has a [UILO](../../UILO.hpp.md#uilo) to load it through. With an aspect lock set, the element's declared size is also rewritten here from the source dimensions, so the [Modifier](../Modifier.hpp.md#modifier) itself carries the derived axis and the parent reserves the right amount of space for it. A pixel-sized axis is required for that: a percent one has no fixed number to derive from and is left to the layout.

---

### rebuildTexture

```cpp
rebuildTexture()
```

**Returns** — void

Drops everything derived from the old file -- the private texture if there was one, the CPU pixel buffer, and the loaded state -- and reloads from the current path. Called when the options are replaced, since the path may have changed.

---

### isLoaded

```cpp
isLoaded()
```

**Returns** — bool

Whether the texture loaded. False until the first update after the element is bound to a [UILO](../../UILO.hpp.md#uilo), and permanently false when the file could not be read.

---

### releaseOwnedTexture

```cpp
releaseOwnedTexture()
```

**Returns** — void

Destroys the texture only when this [Image](Image.hpp.md#image) owns it. Cached textures belong to the [Renderer](../../renderer/RendererImpl.hpp.md#renderer) and are shared with every other [Image](Image.hpp.md#image) naming the same file, so only a private copy-on-write texture is ours to destroy.

---

### ensurePixels

```cpp
ensurePixels()
```

**Returns** — bool -- true when a CPU-side pixel buffer is available

Decodes the source file into a CPU-side RGBA8 buffer the first time pixel access needs it. Already-decoded buffers are kept, so repeated reads cost nothing after the first.

---

### getPixel

```cpp
getPixel(const uint32_t x, const uint32_t y)
```

**Parameters**

- `const uint32_t x`
- `const uint32_t y`

**Returns** — [Color](../../utils/Color.hpp.md#color) -- transparent when unavailable or out of bounds

Reads one texel from the source image, decoding it on first use. Coordinates are in source texels and are unaffected by on-screen size, flips or recolor.

---

### setPixel

```cpp
setPixel(const uint32_t x, const uint32_t y, const Color& color)
```

**Parameters**

- `const uint32_t x`
- `const uint32_t y`
- `const Color& color`

**Returns** — void

Writes one texel into the CPU-side buffer and marks it for upload. The write lands on the GPU at the next render rather than immediately, so setting many pixels in a frame costs one upload. Out-of-bounds writes are ignored.

---

### syncPixels

```cpp
syncPixels()
```

**Returns** — void

Uploads pending pixel writes, detaching onto a private texture the first time. The path-cached texture is shared with any other [Image](Image.hpp.md#image) using the same file and is immutable in bgfx anyway, so the first write creates a private mutable texture of its own and this [Image](Image.hpp.md#image) takes ownership of it from then on.

---

### update

```cpp
update(Rectf& parentBounds, float dt)
```

**Parameters**

- `Rectf& parentBounds`
- `float dt`

**Returns** — void

Loads the texture if that has not happened yet, resolves the element's bounds, and then overrides one axis from the source aspect ratio when an aspect lock is set. The override happens after resize rather than instead of it, so the locked axis is measured against whatever the layout actually gave the other one.

---

### render

```cpp
render()
```

**Returns** — void

Uploads any pending pixel writes and draws the textured quad, with the flips and elliptical clip the options ask for. The upload runs before the loaded check on purpose: a setPixel made before the first frame creates the texture rather than needing one to already exist. The tint is white unless recolor is on, so an ordinary image is drawn at its own colours.
