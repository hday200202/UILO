# Gradient.hpp

`include/utils/Gradient.hpp`

[← index](../README.md)

## Types

- [GradientColor](#gradientcolor)
- [Gradient](#gradient)

---

### GradientColor

One corner of a gradient: a literal [Color](Color.hpp.md#color), or a palette role resolved at draw time. Converts implicitly from [Color](Color.hpp.md#color) and from a role string, so call sites just pass a color or a role name.

---

### Gradient

A per-corner background fill, built once and shared across elements the same way a [Material](Material.hpp.md#material) is. Each of the four corners is a [GradientColor](#gradientcolor) -- a literal [Color](Color.hpp.md#color) or a palette role, freely mixable, and since [GradientColor](#gradientcolor) converts implicitly from both you never name the type at a call site. Colors are interpolated across the background quad on the GPU and clipped by the same rounded-corner mask a solid fill uses, so gradients and rounding compose at no extra cost.

> The fluent setters name the position each colour occupies, so a gradient reads without having to remember corner order:

```cpp
RowOptions().setGradient({tl, tr, bl, br});
RowOptions().setGradient({"accent", "accent", "panel", "panel"});
Gradient().setTop(Color{60, 40, 120}).setBottom(Color{20, 20, 40});
Gradient().setLeft("accent").setRight("panel");
```


> vertical() and horizontal() are shorthand for the two common cases. A whole gradient can also be named in the [Palette](../Palette.hpp.md#palette) with setGradient("hero", g) and referenced per element with setGradientRole("hero"), which is what lets a palette switch restyle every gradient at once.
