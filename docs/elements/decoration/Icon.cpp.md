# Icon.cpp

`include/elements/decoration/Icon.cpp`

[← index](../../README.md)

## Functions

- [`rasterizer()`](#rasterizer)
- [`packColor(Color c)`](#packcolor)
- [`parseViewBox(std::string_view markup, float& w, float& h)`](#parseviewbox)
- [`Icon(Modifier modifier, IconOptions options, const std::string& name)`](#icon)
- [`~Icon()`](#icon)
- [`setOptions(const IconOptions& opts)`](#setoptions)
- [`releaseTexture()`](#releasetexture)
- [`getMarkup()`](#getmarkup)
- [`getSourceAspect()`](#getsourceaspect)
- [`ensureRaster(uint32_t pxW, uint32_t pxH, Color tint)`](#ensureraster)
- [`update(Rectf& parentBounds, float dt)`](#update)
- [`render()`](#render)

---

### rasterizer

```cpp
rasterizer()
```

**Returns** — NSVGrasterizer* -- null when one could not be created

The one rasterizer for the process. Rasterizing is single- threaded here, running from render(), and the context holds reusable scratch buffers, so building a fresh one per icon would throw those away. Destroyed with the holder at exit.

---

### packColor

```cpp
packColor(Color c)
```

**Parameters**

- `Color c`

**Returns** — uint32_t

Packs a colour the way NanoSVG stores one, as 0xAABBGGRR.

---

### parseViewBox

```cpp
parseViewBox(std::string_view markup, float& w, float& h)
```

**Parameters**

- `std::string_view markup`
- `float& w`
- `float& h`

**Returns** — bool -- true when four usable numbers were found

Pulls the four viewBox numbers straight out of the markup. NanoSVG applies the viewBox transform during parsing and does not report it back, but the original units are needed for two things: the icon's intrinsic aspect ratio, and converting an authored stroke width into the parsed, already-scaled units NSVGshape carries.

---

### Icon

```cpp
Icon(Modifier modifier, IconOptions options, const std::string& name)
```

**Parameters**

- `Modifier modifier`
- `IconOptions options`
- `const std::string& name`

**Returns** — [Icon](Icon.hpp.md#icon)

Constructs an icon from a modifier and its options. Nothing is parsed or rasterized here: the source is resolved and rendered on the first draw, once the element has a renderer and a size.

---

### ~Icon

```cpp
~Icon()
```

Destroys the icon's raster texture. Unlike [Image](Image.hpp.md#image), an [Icon](Icon.hpp.md#icon) always owns its texture, since it rasterizes its own pixels rather than sharing a cached file.

---

### setOptions

```cpp
setOptions(const IconOptions& opts)
```

**Parameters**

- `const IconOptions& opts`

**Returns** — void

Replaces the options and clears every cache key, forcing a re- raster on the next draw, since the source, colour and stroke may all have moved at once.

---

### releaseTexture

```cpp
releaseTexture()
```

**Returns** — void

Destroys the raster texture if there is one, leaving the icon ready to rasterize again.

---

### getMarkup

```cpp
getMarkup()
```

**Returns** — std::string_view -- empty when the source is unset or unknown

The markup this icon resolves to, which is also what an alternate renderer emits. The three sources are checked in order: literal markup, then a file, then a registry name. A file is read once and held, keyed by its path, because the returned view has to stay valid after this call returns; a failed read reports the error once and yields empty markup, so a missing file draws nothing rather than retrying every frame.

---

### getSourceAspect

```cpp
getSourceAspect()
```

**Returns** — float -- width over height, 1.0 when it could not be determined

The icon's intrinsic aspect ratio, read from the markup's viewBox and cached, so a caller sizing an element from its art does not re-scan the markup every frame.

---

### ensureRaster

```cpp
ensureRaster(uint32_t pxW, uint32_t pxH, Color tint)
```

**Parameters**

- `uint32_t pxW`
- `uint32_t pxH`
- `Color tint`

**Returns** — bool -- true when a texture is ready to draw

Rasterizes the icon at a pixel size, reusing the existing texture when nothing it depends on has changed. The cache key covers only what affects the pixels -- size, tint, stroke width, the preserve flag and the source markup -- so flips and the destination rect are left to draw time and never force a re- raster. Recoloring retints whichever paints a shape already uses; a shape the author left unpainted stays that way. The authored stroke width is folded through the viewBox scale NanoSVG has already applied, without which a width of 1.5 on a 24-unit grid would come out several times too thin. The texture is reused when only its contents changed and reallocated only when its dimensions move. Returns false for an empty source, a zero size, a texture that would exceed what bgfx can address, or a failed allocation, in each case leaving the icon undrawn rather than partly drawn.

---

### update

```cpp
update(Rectf& parentBounds, float dt)
```

**Parameters**

- `Rectf& parentBounds`
- `float dt`

**Returns** — void

Resolves the icon's bounds. Rasterizing waits for render, which is where the on-screen size is finally known.

---

### render

```cpp
render()
```

**Returns** — void

Resolves the tint through the [Palette](../../Palette.hpp.md#palette), rasterizes at the size the icon actually covers on screen -- which is what keeps it sharp through a scale or DPI change instead of resampling a fixed raster -- and draws it. With preserveAspect on, the aspect-correct raster is letterboxed and centred in the bounds rather than stretched by the quad. The draw passes white because the tint is already baked into the pixels.
