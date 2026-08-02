# Math.hpp

`include/utils/Math.hpp`

[← index](../README.md)

## Types

- [Vec2f](#vec2f)
- [Vec2u](#vec2u)
- [Vec2i](#vec2i)
- [Rectf](#rectf)

---

### Vec2f

A two-component float vector, used for positions, sizes and deltas throughout the library.

---

### Vec2u

An unsigned pair, for pixel dimensions that cannot be negative such as a window or texture size.

---

### Vec2i

A signed integer pair, for whole-pixel coordinates.

---

### Rectf

An axis-aligned rectangle held as a position and a size rather than two corners, which is the form layout works in. contains() treats the rectangle as half-open -- the left and top edges are inside and the right and bottom are not -- so two rectangles sharing an edge never both claim the same pixel, and a click on a boundary lands in exactly one of them.
