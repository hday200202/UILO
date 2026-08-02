# Renderer_Texture.cpp

`include/renderer/Renderer_Texture.cpp`

[← index](../README.md)

## Functions

- [`loadTexture(const std::string& path)`](#loadtexture)
- [`loadImagePixels(...)`](#loadimagepixels)
- [`createTexture(uint16_t width, uint16_t height)`](#createtexture)
- [`updateTexture(const Texture& tex, const uint8_t* rgba)`](#updatetexture)
- [`destroyTexture(Texture& tex)`](#destroytexture)
- [`drawImage(...)`](#drawimage)
- [`drawGlass(const Rectf& dst, const Material& mat, Color baseColor)`](#drawglass)

---

### loadTexture

```cpp
loadTexture(const std::string& path)
```

**Parameters**

- `const std::string& path`

**Returns** — Texture

applyScissor / scissorEmpty / clip-uniform helpers are shared inlines in RendererImpl.hpp.

---

### loadImagePixels

```cpp
loadImagePixels(...)
```

**Parameters**

- `const std::string& path`
- `std::vector<uint8_t>& outRgba`
- `uint32_t& outWidth`
- `uint32_t& outHeight`

**Returns** — bool

Decodes an image file into a CPU-side RGBA8 buffer without creating a texture, which is what backs [Image](../elements/decoration/Image.hpp.md#image)'s get and set pixel access.

---

### createTexture

```cpp
createTexture(uint16_t width, uint16_t height)
```

**Parameters**

- `uint16_t width`
- `uint16_t height`

**Returns** — Texture

Creates an empty mutable RGBA8 texture, for content the application updates itself rather than loading from a file.

---

### updateTexture

```cpp
updateTexture(const Texture& tex, const uint8_t* rgba)
```

**Parameters**

- `const Texture& tex`
- `const uint8_t* rgba`

Uploads a full RGBA8 buffer into a texture created by createTexture.

---

### destroyTexture

```cpp
destroyTexture(Texture& tex)
```

**Parameters**

- `Texture& tex`

Destroys a texture and invalidates the handle. Only textures the caller owns should be passed: a path-cached one belongs to the renderer and is shared.

---

### drawImage

```cpp
drawImage(...)
```

**Parameters**

- `const Rectf& dst`
- `const Texture& tex`
- `Color tint`
- `Rectf uv`
- `bool flipH`
- `bool flipV`
- `bool clipEllipse`

Draws a texture into a rectangle, with a tint, a source sub- rectangle, optional horizontal and vertical flips, and an optional elliptical mask for a round avatar.

---

### drawGlass

```cpp
drawGlass(const Rectf& dst, const Material& mat, Color baseColor)
```

**Parameters**

- `const Rectf& dst`
- `const Material& mat`
- `Color baseColor`

------------------------------------------------------------ --------------- drawGlass — sample the blurred backdrop (built in the previous frame) and overlay tint + edge highlight via the fs_glass shader. ------------------------- --------------------------------------------------
