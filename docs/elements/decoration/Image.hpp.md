# Image.hpp

`include/elements/decoration/Image.hpp`

[← index](../../README.md)

## Types

- [ImageOptions](#imageoptions)
- [Image](#image)

---

### ImageOptions

Everything an [Image](#image) draws: the file it comes from, an optional tint, whether either axis is driven by the source aspect ratio, and the per-draw transforms -- horizontal and vertical flips, and an elliptical clip for a round avatar. The tint only applies with recolor on, and comes as a literal plus a role, where the role wins when it resolves against the active [Palette](../../Palette.hpp.md#palette).

> Locking an aspect makes one axis follow the other rather than the slot the parent gave it, so only one of the two lock flags is meaningful at a time.

---

### Image

A textured quad loaded from a file. The texture is loaded on the first update, once the element is bound to a [UILO](../../UILO.hpp.md#uilo) and so has a renderer to load it through, and is cached by path -- so several Images naming the same file share one texture. Either axis can be driven by the source aspect ratio instead of by the slot the parent gave it.

> getPixel and setPixel work in source-texture texels: (0,0) is the image's top-left, x below getTextureWidth() and y below getTextureHeight(), and neither on-screen size nor flips nor recolor affects them. Reads out of bounds, or before the element is attached to a [UILO](../../UILO.hpp.md#uilo), return transparent; writes there are ignored.

> The first pixel access decodes the file into a CPU-side buffer. The first *write* then detaches this Image onto a private texture, so the other Images sharing that file are unaffected, and bgfx's immutable cached texture is left alone. Writes are batched and uploaded once per frame at render time.
