# Resizer.cpp

`include/elements/interactible/Resizer.cpp`

[← index](../../README.md)

## Functions

- [`Resizer(Modifier modifier, ResizerOptions options, const std::string& name)`](#resizer)
- [`update(Rectf& parentBounds, float dt)`](#update)
- [`render()`](#render)
- [`checkHover(const Vec2f& mousePosition)`](#checkhover)
- [`checkLeftClick(const Vec2f& mousePosition)`](#checkleftclick)
- [`onDeactivate()`](#ondeactivate)

---

### Resizer

```cpp
Resizer(Modifier modifier, ResizerOptions options, const std::string& name)
```

**Parameters**

- `Modifier modifier`
- `ResizerOptions options`
- `const std::string& name`

**Returns** — [Resizer](Resizer.hpp.md#resizer)

Constructs a drag handle and tags it as a [Resizer](Resizer.hpp.md#resizer), which is what makes [Row](../containers/Row.hpp.md#row) and [Column](../containers/Column.hpp.md#column) skip it in the flow and place it at a boundary instead.

---

### update

```cpp
update(Rectf& parentBounds, float dt)
```

**Parameters**

- `Rectf& parentBounds`
- `float dt`

**Returns** — void

Takes the bounds the parent worked out and, while a drag is in progress, resizes the target. The parent's rect is used verbatim rather than run through resize(), because [Row](../containers/Row.hpp.md#row) and [Column](../containers/Column.hpp.md#column) place a resizer at a boundary rather than in the flow and have already decided exactly where it goes. The drag ends when the mouse button is found released, which is polled here rather than driven by an event so a release outside the window still finishes it. Deltas are converted out of render pixels into unscaled units before being applied, since a [Modifier](../Modifier.hpp.md#modifier)'s dimensions are unscaled, and each direction clamps to its own min and max and quantises to the step when one is set.

---

### render

```cpp
render()
```

**Returns** — void

Draws the visible strip, which is narrower than the hit area and centred within it -- the handle is easy to grab but reads as a thin divider. Nothing is drawn when the colour is transparent, which is the default: a resizer is usually invisible until a hover handler colours it in.

---

### checkHover

```cpp
checkHover(const Vec2f& mousePosition)
```

**Parameters**

- `const Vec2f& mousePosition`

**Returns** — bool -- true whenever the pointer is inside

Tracks the hovered state, fires the enter and exit handlers, and asks for the resize cursor matching the drag axis. The request is made at a higher priority than an ordinary element's, so the resize arrows win over the hand of whatever the handle straddles.

---

### checkLeftClick

```cpp
checkLeftClick(const Vec2f& mousePosition)
```

**Parameters**

- `const Vec2f& mousePosition`

**Returns** — bool -- true when the handle took the click

Starts a drag, or restores the target's original size on a double click. The starting size is read from the target's resolved bounds plus its outer padding, since padding is inside the slot the parent gave it and a drag should move the slot edge rather than the drawn edge. Declines when there is no target, which is the case for a handle with no visible neighbour on the side it points at.

---

### onDeactivate

```cpp
onDeactivate()
```

**Returns** — void

Ends any drag in progress when focus moves elsewhere, so a handle cannot be left latched after a click lands on something else.
