# Renderer_Text.cpp

`include/renderer/Renderer_Text.cpp`

[← index](../README.md)

## Functions

- [`readFile(const char* path, std::vector&lt;uint8_t&gt;& out)`](#readfile)
- [`initAtlasFace(...)`](#initatlasface)
- [`getFace(uint32_t fontId, float pixelHeight)`](#getface)
- [`getGlyph(FontFace& face, uint32_t codepoint)`](#getglyph)
- [`loadFont(const std::string& path)`](#loadfont)
- [`measureText(const std::string& utf8, const Font& font, float sizePx)`](#measuretext)
- [`charPositions(const std::string& utf8, const Font& font, float sizePx)`](#charpositions)
- [`drawText(...)`](#drawtext)

---

### readFile

```cpp
readFile(const char* path, std::vector<uint8_t>& out)
```

**Parameters**

- `const char* path`
- `std::vector<uint8_t>& out`

**Returns** — bool

Slurp a file fully into a vector.

---

### initAtlasFace

```cpp
initAtlasFace(...)
```

**Parameters**

- `FontFace& face`
- `stbtt_fontinfo info`
- `std::vector<uint8_t> ttf`
- `float pixelHeight`

Sets up a font face at one pixel size: reads the vertical metrics, scales them, and allocates a blank single-channel atlas the glyphs are packed into on demand.

---

### getFace

```cpp
getFace(uint32_t fontId, float pixelHeight)
```

**Parameters**

- `uint32_t fontId`
- `float pixelHeight`

**Returns** — FontFace*

The cached face for a font at a pixel size, created on first use. Sizes are cached separately because a glyph atlas is rasterised at one size; asking for a new size builds a new face rather than scaling an existing one, which would blur it.

---

### getGlyph

```cpp
getGlyph(FontFace& face, uint32_t codepoint)
```

**Parameters**

- `FontFace& face`
- `uint32_t codepoint`

**Returns** — const Glyph*

The cached glyph for a codepoint, rasterised and packed into the atlas on first use. Packing runs left to right in rows; when the atlas is full the glyph is recorded as zero-sized so it simply does not draw, rather than corrupting the sheet.

---

### loadFont

```cpp
loadFont(const std::string& path)
```

**Parameters**

- `const std::string& path`

**Returns** — [Font](Renderer.hpp.md#font)

Loads a TTF by path, cached so the same file is read once. A missing or invalid file falls back to the embedded face and is cached under the requested path too, so a bad path warns once rather than retrying every frame.

---

### measureText

```cpp
measureText(const std::string& utf8, const Font& font, float sizePx)
```

**Parameters**

- `const std::string& utf8`
- `const Font& font`
- `float sizePx`

**Returns** — [TextMetrics](Renderer.hpp.md#textmetrics)

Measures a string's bounding box and vertical metrics at a size, walking the same glyph advances the draw uses so the two always agree.

---

### charPositions

```cpp
charPositions(const std::string& utf8, const Font& font, float sizePx)
```

**Parameters**

- `const std::string& utf8`
- `const Font& font`
- `float sizePx`

**Returns** — std::vector&lt;[Vec2f](../utils/Math.hpp.md#vec2f)&gt;

The position of every codepoint boundary in a string, N+1 of them so the trailing cursor slot has one. This is what the [Textbox](../elements/interactible/Textbox.hpp.md#textbox) maps a caret and a hit test through.

---

### drawText

```cpp
drawText(...)
```

**Parameters**

- `const std::string& utf8`
- `Vec2f position`
- `const Font& font`
- `float sizePx`
- `Color color`
- `TextStyle style`

Draws a UTF-8 string, one textured quad per glyph, batched into a single submit. Bold re-emits each glyph nudged sideways and italic shears the quad about the baseline, both synthetic so any face can be styled without a second font file, and neither changes the advances -- so a styled run measures and wraps exactly like a plain one.
