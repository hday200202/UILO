# Modifier.cpp

`include/elements/Modifier.cpp`

[← index](../README.md)

## Functions

- [`setWidth(Dimension dim)`](#setwidth)
- [`setHeight(Dimension dim)`](#setheight)
- [`setAlign(Align alignment)`](#setalign)
- [`setVisible(bool visible)`](#setvisible)
- [`ignoreScroll(bool ignore)`](#ignorescroll)
- [`setFreePosition(const Vec2f& freePos)`](#setfreeposition)
- [`setMaterial(const Material& m)`](#setmaterial)
- [`setCursor(CursorType cursor)`](#setcursor)
- [`clearCursor()`](#clearcursor)
- [`getWidth()`](#getwidth)
- [`getHeight()`](#getheight)
- [`getAlign()`](#getalign)
- [`getOnLeftClick()`](#getonleftclick)
- [`getOnRightClick()`](#getonrightclick)
- [`getOnHoverEnter()`](#getonhoverenter)
- [`getOnHoverExit()`](#getonhoverexit)
- [`getOnUpdateStart()`](#getonupdatestart)
- [`getOnUpdateEnd()`](#getonupdateend)
- [`getOnScroll()`](#getonscroll)
- [`getVisible()`](#getvisible)
- [`getIgnoreScroll()`](#getignorescroll)
- [`getFreePosition()`](#getfreeposition)
- [`getMaterial()`](#getmaterial)
- [`getCursor()`](#getcursor)
- [`hasCursor()`](#hascursor)

---

### setWidth

```cpp
setWidth(Dimension dim)
```

**Parameters**

- `Dimension dim`

**Returns** — [Modifier](Modifier.hpp.md#modifier)&

Sets the element's width. A percent value is clamped to 1..100, since a container distributes percent children against the space left over and a value outside that range has no meaning there. Pixel values pass through and are scaled at layout time.

---

### setHeight

```cpp
setHeight(Dimension dim)
```

**Parameters**

- `Dimension dim`

**Returns** — [Modifier](Modifier.hpp.md#modifier)&

Sets the element's height, clamping a percent value the same way setWidth does.

---

### setAlign

```cpp
setAlign(Align alignment)
```

**Parameters**

- `Align alignment`

**Returns** — [Modifier](Modifier.hpp.md#modifier)&

Sets where the element sits inside the slot its parent gives it. Horizontal and vertical flags combine with `|`.

---

### setVisible

```cpp
setVisible(bool visible)
```

**Parameters**

- `bool visible`

**Returns** — [Modifier](Modifier.hpp.md#modifier)&

Shows or hides the element. A hidden element is skipped by layout, takes no space, and is not drawn.

---

### ignoreScroll

```cpp
ignoreScroll(bool ignore)
```

**Parameters**

- `bool ignore`

**Returns** — [Modifier](Modifier.hpp.md#modifier)&

Pins the element inside a scrollable parent. A pinned child keeps its place and reserves its space, and the parent scrolls only the viewport left over -- which is how a header stays put above a scrolling list.

---

### setFreePosition

```cpp
setFreePosition(const Vec2f& freePos)
```

**Parameters**

- `const Vec2f& freePos`

**Returns** — [Modifier](Modifier.hpp.md#modifier)&

Position for an element placed outside the layout flow.

---

### setMaterial

```cpp
setMaterial(const Material& m)
```

**Parameters**

- `const Material& m`

**Returns** — [Modifier](Modifier.hpp.md#modifier)&

Gives the element a material background -- glass and the like. A material owns the background: it draws its own rounded rect, tint and effect, so the element's flat fill is skipped rather than drawn underneath it.

---

### setCursor

```cpp
setCursor(CursorType cursor)
```

**Parameters**

- `CursorType cursor`

**Returns** — [Modifier](Modifier.hpp.md#modifier)&

The mouse cursor to show while the pointer is over this element. Requested for every frame the element is hovered, not once on the way in, so it survives as long as the pointer stays. That is the difference from calling [UILO](../UILO.hpp.md#uilo)::requestCursor() from an onHoverEnter handler: the request pool is cleared at the top of each frame, so an edge-triggered handler's cursor lasts a single frame no matter what priority it asks for.

> An element with a left-click handler already asks for the hand, so this is only needed to pick a different shape, or to force one on an element that is not clickable. Setting Arrow explicitly suppresses that automatic hand.

---

### clearCursor

```cpp
clearCursor()
```

**Returns** — [Modifier](Modifier.hpp.md#modifier)&

Drops an explicit cursor, so the element goes back to the default behaviour -- the hand when it is clickable, and whatever is underneath otherwise.

---

### getWidth

```cpp
getWidth()
```

**Returns** — [Dimension](../utils/Dimension.hpp.md#dimension)

The element's declared width, in pixels or percent.

---

### getHeight

```cpp
getHeight()
```

**Returns** — [Dimension](../utils/Dimension.hpp.md#dimension)

The element's declared height, in pixels or percent.

---

### getAlign

```cpp
getAlign()
```

**Returns** — [Align](../utils/Alignment.hpp.md#align)

The combined alignment flags.

---

### getOnLeftClick

```cpp
getOnLeftClick()
```

**Returns** — const FuncPtr&

The left-click handler, empty when none was set. Testing it is also how an element decides whether it claims pointer events.

---

### getOnRightClick

```cpp
getOnRightClick()
```

**Returns** — const FuncPtr&

The right-click handler, empty when none was set.

---

### getOnHoverEnter

```cpp
getOnHoverEnter()
```

**Returns** — const FuncPtr&

The hover-enter handler, empty when none was set.

---

### getOnHoverExit

```cpp
getOnHoverExit()
```

**Returns** — const FuncPtr&

The hover-exit handler, empty when none was set.

---

### getOnUpdateStart

```cpp
getOnUpdateStart()
```

**Returns** — const FuncPtr&

The start-of-tick hook, empty when none was set.

---

### getOnUpdateEnd

```cpp
getOnUpdateEnd()
```

**Returns** — const FuncPtr&

The end-of-tick hook, empty when none was set.

---

### getOnScroll

```cpp
getOnScroll()
```

**Returns** — const ScrollFuncPtr&

The scroll handler, empty when none was set.

---

### getVisible

```cpp
getVisible()
```

**Returns** — bool

Whether the element is laid out and drawn.

---

### getIgnoreScroll

```cpp
getIgnoreScroll()
```

**Returns** — bool

Whether the element is pinned inside a scrollable parent.

---

### getFreePosition

```cpp
getFreePosition()
```

**Returns** — [Vec2f](../utils/Math.hpp.md#vec2f)

The position for an element placed outside the layout flow.

---

### getMaterial

```cpp
getMaterial()
```

**Returns** — const [Material](../utils/Material.hpp.md#material)&

The element's material. Kind None means it has none, and the ordinary background is drawn instead.

---

### getCursor

```cpp
getCursor()
```

**Returns** — [CursorType](../utils/Cursor.hpp.md#cursortype)

The explicit cursor for this element, Arrow when none was set. Ask hasCursor() to tell a deliberate Arrow from the default.

---

### hasCursor

```cpp
hasCursor()
```

**Returns** — bool

Whether this element names its own cursor, which is what lets an explicit Arrow override the hand a clickable element would otherwise ask for.
