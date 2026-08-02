# RendererImpl.hpp

`include/renderer/RendererImpl.hpp`

[← index](../README.md)

## Types

- [PosColorVertex](#poscolorvertex)
- [PosColorUvVertex](#poscoloruvvertex)
- [Renderer](#renderer)

---

### PosColorVertex

Vertex layout for untextured geometry: a screen-space position and a packed colour. Used by every solid shape.

---

### PosColorUvVertex

Vertex layout for textured geometry, adding texture coordinates. Used by images, glyphs and framebuffer composites.

---

### Renderer

The renderer's private state, kept out of the public header so bgfx and SDL types never leak into anything that includes Renderer.hpp. Holds the shader programs and uniforms, the offscreen targets and blur ladder, the clip and rotation stacks, the solid-geometry batch, and the texture and font caches.
