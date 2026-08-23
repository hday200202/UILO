# Button.hpp

`include/elements/interactible/Button.hpp`

[← index](../../README.md)

## Types

- [ButtonOptions](#buttonoptions)
- [Button](#button)

## Functions

- [`inheritRounding(const std::optional&lt;float&gt;& own, float fallback)`](#inheritrounding)
- [`getRounding()`](#getrounding)

---

### ButtonOptions

Everything a [Button](#button) draws: background fill or gradient, corner rounding, an inside border, and the [Text](../decoration/Text.hpp.md#text) element used as its label. Colors come as a literal plus a role, where the role wins when it resolves against the active [Palette](../../Palette.hpp.md#palette) and the literal is the fallback. These are a curated subset of [RowOptions](../containers/Row.hpp.md#rowoptions), since a [Button](#button) is a [Row](../containers/Row.hpp.md#row) -- scrolling and the subdivision grid are deliberately not exposed.

---

### Button

A clickable [Row](../containers/Row.hpp.md#row). Being a [Row](../containers/Row.hpp.md#row) rather than an [Interactible](Interactible.hpp.md#interactible) is deliberate: a button has no state to keep between clicks, and it means the label goes in as an ordinary child, so anything else can be added beside it -- which is how [Dropdown](Dropdown.hpp.md#dropdown) builds its header and arrow. It claims a press whether or not a callback was attached, so a click never falls through to the panel behind, and it asks for the hand cursor whenever the pointer is over it.

> [ButtonOptions](#buttonoptions) are pushed down into the underlying [RowOptions](../containers/Row.hpp.md#rowoptions) on every draw, so a handler that mutates getOptions() directly is reflected on the next frame without calling setOptions().

---

### inheritRounding

```cpp
inheritRounding(const std::optional<float>& own, float fallback)
```

**Parameters**

- `const std::optional<float>& own`
- `float fallback`

**Returns** — [ButtonOptions](#buttonoptions)&

Takes a rounding straight from a composite widget. `own` is whatever the widget was told, empty when nothing, and `fallback` is that widget's own default for when the theme is silent too. Passing it through unresolved, rather than as a number the widget already worked out, is what lets the theme keep reaching this element after it has been built.

> An empty `own` leaves the field following the theme rather than pinning it empty, so a themed radius still reaches the part.

---

### getRounding

```cpp
getRounding()
```

**Returns** — float

Corner radius, resolved in three steps: the value this element was given, then the active [Theme](../../utils/Theme.hpp.md#theme)'s, then the fallback carried by inheritRounding. Resolved on every read rather than cached, so changing the [Theme](../../utils/Theme.hpp.md#theme) restyles a button already on screen.
