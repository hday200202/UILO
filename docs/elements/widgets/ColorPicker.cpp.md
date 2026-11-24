# ColorPicker.cpp

`include/elements/widgets/ColorPicker.cpp`

[← index](../../README.md)

## Types

- [Preview](#preview)
- [Field](#field)
- [ColorPicker](#colorpicker)
- [setColor](#setcolor)
- [parseHex](#parsehex)

---

### Preview

Shows the current RGBA value over a checkerboard, including full transparency.

---

### Field

Absolute-position editing keeps each gesture on its original rail, clamps out-of-bounds drags, and never warps or hides the pointer.

---

### ColorPicker

Builds a stable child tree so changing values never recreates focused controls.

---

### setColor

Silently adopts an RGBA value, retaining hue for gray and hue/saturation for black.

---

### parseHex

Strictly accepts RGB/RGBA shorthand and full hex, with an optional leading #.
