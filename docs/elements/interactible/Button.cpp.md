# Button.cpp

`include/elements/interactible/Button.cpp`

[← index](../../README.md)

## Functions

- [`rowOptionsFrom(const ButtonOptions& o)`](#rowoptionsfrom)
- [`Button(Modifier modifier, ButtonOptions options, const std::string& name)`](#button)
- [`setOptions(const ButtonOptions& opts)`](#setoptions)
- [`checkLeftClick(const Vec2f& mousePosition)`](#checkleftclick)
- [`checkRightClick(const Vec2f& mousePosition)`](#checkrightclick)
- [`checkHover(const Vec2f& mousePosition)`](#checkhover)
- [`checkScroll(const Vec2f& mousePosition, float delta, bool precise, bool momentum)`](#checkscroll)
- [`render()`](#render)

---

### rowOptionsFrom

```cpp
rowOptionsFrom(const ButtonOptions& o)
```

**Parameters**

- `const ButtonOptions& o`

**Returns** — [RowOptions](../containers/Row.hpp.md#rowoptions)

The one place [ButtonOptions](Button.hpp.md#buttonoptions) becomes [RowOptions](../containers/Row.hpp.md#rowoptions). Three callers need the conversion -- the constructor, setOptions, and the per- frame sync in render -- and they were once written out separately, which is exactly how a newly added property ends up wired in one of them and forgotten in the other two. Rounding is passed through unresolved so the Theme keeps reaching the row.

---

### Button

```cpp
Button(Modifier modifier, ButtonOptions options, const std::string& name)
```

**Parameters**

- `Modifier modifier`
- `ButtonOptions options`
- `const std::string& name`

**Returns** — [Button](Button.hpp.md#button)

Constructs a button as a [Row](../containers/Row.hpp.md#row) carrying the converted options, and adds the label as its first child when one was given. Tagged as a [Button](Button.hpp.md#button) so hit-testing and the web bridge can identify it.

---

### setOptions

```cpp
setOptions(const ButtonOptions& opts)
```

**Parameters**

- `const ButtonOptions& opts`

**Returns** — void

Replaces the options, pushes them into the underlying [Row](../containers/Row.hpp.md#row), and rebuilds the child list around the new label. The child list is cleared outright, so anything added beside the label is dropped -- a widget that keeps extra children re-adds them afterwards.

---

### checkLeftClick

```cpp
checkLeftClick(const Vec2f& mousePosition)
```

**Parameters**

- `const Vec2f& mousePosition`

**Returns** — bool -- true when the button claimed the click

Uses [Element](../Element.hpp.md#element)'s handling rather than [Row](../containers/Row.hpp.md#row)'s, so the click stops here instead of being offered to the label and the other children first. A button is one target, not a container of them.

---

### checkRightClick

```cpp
checkRightClick(const Vec2f& mousePosition)
```

**Parameters**

- `const Vec2f& mousePosition`

**Returns** — bool -- true when the button claimed the click

As checkLeftClick, for the right button.

---

### checkHover

```cpp
checkHover(const Vec2f& mousePosition)
```

**Parameters**

- `const Vec2f& mousePosition`

**Returns** — bool -- true when the button claimed the hover

Asks for the hand cursor whenever the pointer is inside, which is unconditional here rather than tied to having a click handler: anything drawn as a button should read as clickable. The request is made every frame, since [UILO](../../UILO.hpp.md#uilo) clears the pool each frame.

---

### checkScroll

```cpp
checkScroll(const Vec2f& mousePosition, float delta, bool precise, bool momentum)
```

**Parameters**

- `const Vec2f& mousePosition`
- `float delta`
- `bool precise`
- `bool momentum`

**Returns** — bool -- true when the event was consumed

Uses [Element](../Element.hpp.md#element)'s handling, so a button never scrolls itself and a wheel over one bubbles to the list or panel it sits in.

---

### render

```cpp
render()
```

**Returns** — void

Syncs the live [ButtonOptions](Button.hpp.md#buttonoptions) into the underlying [Row](../containers/Row.hpp.md#row) before drawing, so a handler that mutates getOptions() directly -- the usual `b->getOptions().setColorRole("accentHover")` from an onHoverEnter -- takes effect on the next frame without an explicit setOptions() call. The comparison guards against rebuilding the [RowOptions](../containers/Row.hpp.md#rowoptions) every frame, and the row's own scroll settings are carried across since [ButtonOptions](Button.hpp.md#buttonoptions) does not model them.
