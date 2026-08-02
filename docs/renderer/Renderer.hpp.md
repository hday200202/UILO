# Renderer.hpp

`include/renderer/Renderer.hpp`

[← index](../README.md)

## Types

- [Font](#font)
- [TextMetrics](#textmetrics)

---

### Font

An opaque handle to a loaded font, an index into the renderer's font table. Invalid until loadFont succeeds, which is what a caller checks before drawing rather than testing the path.

---

### TextMetrics

Measurements of a laid-out string: the bounding box plus the vertical metrics of the face it was measured with. lineHeight() is ascent plus descent plus the recommended gap, which is the spacing text should be laid out on.
