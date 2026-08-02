# Spacer.cpp

`include/elements/decoration/Spacer.cpp`

[← index](../../README.md)

## Functions

- [`Spacer(Modifier modifier, SpacerOptions options, const std::string& name)`](#spacer)
- [`update(Rectf& parentBounds, float dt)`](#update)
- [`render()`](#render)

---

### Spacer

```cpp
Spacer(Modifier modifier, SpacerOptions options, const std::string& name)
```

**Parameters**

- `Modifier modifier`
- `SpacerOptions options`
- `const std::string& name`

**Returns** — [Spacer](Spacer.hpp.md#spacer)

Constructs a spacer from a modifier and its options, and tags it as a [Spacer](Spacer.hpp.md#spacer) so layout and the web bridge can identify it.

---

### update

```cpp
update(Rectf& parentBounds, float dt)
```

**Parameters**

- `Rectf& parentBounds`
- `float dt`

**Returns** — void

Resolves the spacer's own bounds inside the slot its parent gave it. There is no state to advance, so the frame time is unused.

---

### render

```cpp
render()
```

**Returns** — void

Draws the spacer's fill and outline, and nothing at all in the usual case where it has neither. An outline with no fill is a frame, so a transparent spacer still draws when one is set. The dirty flag is cleared on every path, including the early ones, so a spacer that has nothing to draw does not keep asking to be redrawn.
