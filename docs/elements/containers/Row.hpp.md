# Row.hpp

`include/elements/containers/Row.hpp`

[← index](../../README.md)

## Types

- [RowOptions](#rowoptions)
- [Row](#row)

## Functions

- [`inheritRounding(const std::optional&lt;float&gt;& own, float fallback)`](#inheritrounding)
- [`getRounding()`](#getrounding)
- [`setOptions(const RowOptions& opts)`](#setoptions)

---

### RowOptions

Everything a [Row](#row) draws that is not layout: background fill, gradient, corner rounding, border, scrolling, the subdivision grid and zoom. Colors come as a literal plus a role, where the role wins when it resolves against the active [Palette](../../Palette.hpp.md#palette) and the literal is the fallback. A gradient takes precedence over a flat fill when active, and a [Material](../../utils/Material.hpp.md#material) on the [Modifier](../Modifier.hpp.md#modifier) takes precedence over both. Sizes are unscaled content pixels; [UILO](../../UILO.hpp.md#uilo) multiplies by its scale at layout time.

---

### Row

A [Container](Container.hpp.md#container) that lays its children out left to right, distributing the horizontal axis between them and giving each the full height. Optionally scrolls, zooms, and draws a subdivision grid behind its children. [Column](Column.hpp.md#column) is the same class with the axes exchanged.

---

### inheritRounding

```cpp
inheritRounding(const std::optional<float>& own, float fallback)
```

**Parameters**

- `const std::optional<float>& own`
- `float fallback`

**Returns** — [RowOptions](#rowoptions)&

Takes a rounding straight from a composite widget. `own` is whatever the widget was told, empty when nothing, and `fallback` is that widget's own default for when the theme is silent too. Passing it through unresolved, rather than as a number the widget already worked out, is what lets the theme keep reaching this element after it has been built.

---

### getRounding

```cpp
getRounding()
```

**Returns** — float

Corner radius, resolved in three steps: the value this element was given, then the active Theme's, then the fallback carried by inheritRounding (0 for a plain [Row](#row)). Resolved on every read rather than cached, so changing the Theme restyles an element already on screen.

---

### setOptions

```cpp
setOptions(const RowOptions& opts)
```

**Parameters**

- `const RowOptions& opts`

**Returns** — void

Replaces the row's options and marks it dirty so the new appearance is picked up on the next draw.
