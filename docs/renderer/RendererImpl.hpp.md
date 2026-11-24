# RendererImpl.hpp

`include/renderer/RendererImpl.hpp`

[← index](../README.md)

## Types

- [FontFace](#fontface)
- [Renderer::Impl](#renderer-impl)

---

### FontFace

A font baked at one pixel size. The atlas lives twice: as RGBA8 pixels on the CPU, where glyphs are rasterised as they are first needed, and as a VALO texture the draws sample. Newly packed glyphs mark a dirty rectangle, and the renderer uploads that rectangle once per frame, before the frame's draws run.

---

### Renderer::Impl

The renderer's private state, kept out of the public header so VALO's headers never leak into anything that includes Renderer.hpp. Holds the window and the VALO instance, the one UI program, the per-frame vertex arena and the batch being built in it, the clip and rotation stacks, and the texture and font caches.

> Every shape is tessellated into the arena on the CPU and joins the pending batch when its draw state -- target, mode, texture, clips and scissor -- matches; otherwise the batch is submitted and a new one starts. The arena is uploaded once, just before VALO runs the frame, so every command reads the frame's final contents.
