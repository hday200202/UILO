# Element.hpp

`include/elements/Element.hpp`

[← index](../README.md)

## Types

- [ElementType](#elementtype)
- [Element](#element)

## Functions

- [`takesPointerEvents()`](#takespointerevents)
- [`checkScroll(const Vec2f& mousePosition, Vec2f delta, bool precise, bool momentum)`](#checkscroll)
- [`checkZoom(const Vec2f& mousePosition, float magnification)`](#checkzoom)

---

### ElementType

Which kind of element an instance is, for the cases where behaviour has to branch on it rather than on a virtual call: layout skipping Resizers, hit-testing looking for Interactibles, and the Wt bridge choosing which widget to build. Includes types that are planned but not yet implemented.

---

### Element

Base class for everything [UILO](../UILO.hpp.md#uilo) draws. An element owns its [Modifier](Modifier.hpp.md#modifier) (size, alignment, padding, callbacks), its resolved bounds, and the flags that drive redrawing and deletion. Subclasses supply update() and render(); the pointer-event checks have working defaults that read the [Modifier](Modifier.hpp.md#modifier)'s callbacks. Ownership belongs to [UILO](../UILO.hpp.md#uilo) rather than to the parent: setUILO() puts the element in the element pool, and erase() only marks it, so a handler is free to remove an element while the tree is still being walked. Colors and gradients resolve through the owning [UILO](../UILO.hpp.md#uilo)'s [Palette](../Palette.hpp.md#palette), which is why an unbound element falls back to its literal values.

---

### takesPointerEvents

```cpp
takesPointerEvents()
```

**Returns** — bool

Public read of claimsPointerEvents(), for the web bridge. Native hit-testing stops at the first element that claims a click, so ancestors never see it, but a DOM click bubbles to every ancestor's handler. The bridge stops propagation on exactly the elements that claim, which is what keeps a click on a popup's arrow from also reaching the scrim behind it and dismissing it.

---

### checkScroll

```cpp
checkScroll(const Vec2f& mousePosition, Vec2f delta, bool precise, bool momentum)
```

**Parameters**

- `const Vec2f& mousePosition`
- `Vec2f delta`
- `bool precise`
- `bool momentum`

**Returns** — bool -- true when the event was consumed

Two-axis scroll. Forwards delta.y to the single-axis overload, so every widget is vertical-only without having to say so. Elements that consume both axes -- [Canvas](containers/Canvas.hpp.md#canvas), [Row](containers/Row.hpp.md#row), [Column](containers/Column.hpp.md#column) -- override it.

---

### checkZoom

```cpp
checkZoom(const Vec2f& mousePosition, float magnification)
```

**Parameters**

- `const Vec2f& mousePosition`
- `float magnification`

**Returns** — bool -- true when the gesture was consumed

Pinch or programmatic zoom. `magnification` is an additive per- event ratio, so 0.05 means grow by five percent. Declines by default; [Canvas](containers/Canvas.hpp.md#canvas) and the containers override to consume or forward it.
