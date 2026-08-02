# Interactible.cpp

`include/elements/interactible/Interactible.cpp`

[← index](../../README.md)

## Functions

- [`checkLeftClick(const Vec2f& mousePosition)`](#checkleftclick)
- [`checkRightClick(const Vec2f& mousePosition)`](#checkrightclick)

---

### checkLeftClick

```cpp
checkLeftClick(const Vec2f& mousePosition)
```

**Parameters**

- `const Vec2f& mousePosition`

**Returns** — bool -- true when the click landed on this element

Takes focus and fires the [Modifier](../Modifier.hpp.md#modifier)'s left-click handler. Claiming focus is what deactivates whichever interactible held it before, so an open dropdown closes when a textbox is clicked. The click is always consumed on a hit, handler or not.

---

### checkRightClick

```cpp
checkRightClick(const Vec2f& mousePosition)
```

**Parameters**

- `const Vec2f& mousePosition`

**Returns** — bool -- true when the click landed on this element

As checkLeftClick, for the right button.
