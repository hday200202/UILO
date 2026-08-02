# Canvas.hpp

`include/elements/containers/Canvas.hpp`

[← index](../../README.md)

## Types

- [GridLineStyle](#gridlinestyle)
- [CanvasOptions](#canvasoptions)
- [Canvas](#canvas)

## Functions

- [`getRounding()`](#getrounding)

---

### GridLineStyle

How the [Canvas](#canvas) draws its grid behind the children. None draws nothing, Lines rules the full extent, Dots marks each intersection, and Crosses draws a small tick at each one.

---

### CanvasOptions

Everything a [Canvas](#canvas) draws and how it responds to panning and zooming: the backdrop, the grid metric children snap to, the grid's own appearance, the pan bounds, and the zoom range and locks. Colors come as a literal plus a role, where the role wins when it resolves against the active [Palette](../../Palette.hpp.md#palette). Sizes and coordinates are canvas-space pixels.

---

### Canvas

A [Container](Container.hpp.md#container) that places its children at free canvas-space pixel coordinates inside a pannable, zoomable viewport, rather than flowing them along an axis the way [Row](Row.hpp.md#row) and [Column](Column.hpp.md#column) do. An optional grid metric snaps placement to a regular lattice, and optional per-side bounds clamp how far the view can travel. Pan comes from the trackpad or scroll wheel and, when enabled, a middle-mouse drag; zoom from a pinch or Ctrl-scroll, and can be locked per axis so a timeline can scale horizontally only.

> Child positions live in a side table keyed by element rather than on the children themselves, so an ordinary element can be placed on a canvas without knowing anything about one.

---

### getRounding

```cpp
getRounding()
```

**Returns** — float

Corner radius, resolved in three steps: the value this element was given, then the active Theme's, then 0. Resolved on every read rather than cached, so changing the Theme restyles a canvas already on screen.
