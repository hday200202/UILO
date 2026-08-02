# Gradient.cpp

`include/utils/Gradient.cpp`

[← index](../README.md)

## Functions

- [`vertical(GradientColor top, GradientColor bottom)`](#vertical)
- [`horizontal(GradientColor left, GradientColor right)`](#horizontal)
- [`resolve(const Palette& palette, Color out[4])`](#resolve)

---

### vertical

```cpp
vertical(GradientColor top, GradientColor bottom)
```

**Parameters**

- `GradientColor top`
- `GradientColor bottom`

**Returns** — [Gradient](Gradient.hpp.md#gradient)

A gradient running top to bottom: both top corners take the first colour and both bottom corners the second.

---

### horizontal

```cpp
horizontal(GradientColor left, GradientColor right)
```

**Parameters**

- `GradientColor left`
- `GradientColor right`

**Returns** — [Gradient](Gradient.hpp.md#gradient)

A gradient running left to right: both left corners take the first colour and both right corners the second.

---

### resolve

```cpp
resolve(const Palette& palette, Color out[4])
```

**Parameters**

- `const Palette& palette`
- `Color out[4]`

Resolves the four corner stops into concrete colours, each through its own palette role where it has one. Fills `out` in top-left, top-right, bottom-left, bottom-right order, which is the order the renderer expects its vertex colours in.
