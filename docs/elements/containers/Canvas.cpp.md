# Canvas.cpp

`include/elements/containers/Canvas.cpp`

[← index](../../README.md)

## Functions

- [`Canvas(Modifier modifier, CanvasOptions options, const std::string& name)`](#canvas)
- [`Canvas(...)`](#canvas)
- [`snap(Vec2f v)`](#snap)
- [`clampPan(Vec2f pan)`](#clamppan)
- [`setZoom(float z)`](#setzoom)
- [`setZoom(float zx, float zy)`](#setzoom)
- [`zoomAt(Vec2f pivotWindowPx, float factor)`](#zoomat)
- [`zoomAt(Vec2f pivotWindowPx, float factorX, float factorY)`](#zoomat)
- [`addChild(Element* element, float x, float y)`](#addchild)
- [`setChildPosition(Element* element, float x, float y)`](#setchildposition)
- [`getChildPosition(Element* element)`](#getchildposition)
- [`setPan(Vec2f pan)`](#setpan)
- [`update(Rectf& parentBounds, float dt)`](#update)
- [`render()`](#render)
- [`checkScroll(const Vec2f& mousePosition, float delta, bool precise, bool momentum)`](#checkscroll)
- [`checkScroll(const Vec2f& mousePosition, Vec2f delta, bool precise, bool momentum)`](#checkscroll)
- [`checkZoom(const Vec2f& mousePosition, float magnification)`](#checkzoom)

---

### Canvas

```cpp
Canvas(Modifier modifier, CanvasOptions options, const std::string& name)
```

**Parameters**

- `Modifier modifier`
- `CanvasOptions options`
- `const std::string& name`

**Returns** — [Canvas](Canvas.hpp.md#canvas)

Constructs an empty canvas. Children are added afterwards through addChild, which is what gives them a canvas-space position.

---

### Canvas

```cpp
Canvas(...)
```

**Parameters**

- `Modifier modifier`
- `CanvasOptions options`
- `contains children`
- `const std::string& name`

**Returns** — [Canvas](Canvas.hpp.md#canvas)

Constructs a canvas from a declaration list. Children given this way all land at canvas-space (0, 0), since the list carries no coordinates; move them afterwards with setChildPosition, or use addChild to place them as they are added.

---

### snap

```cpp
snap(Vec2f v)
```

**Parameters**

- `Vec2f v`

**Returns** — [Vec2f](../../utils/Math.hpp.md#vec2f)

Rounds a canvas-space position to the nearest grid intersection. An axis whose grid step is 0 is left alone, so a canvas can snap on one axis and stay free on the other.

---

### clampPan

```cpp
clampPan(Vec2f pan)
```

**Parameters**

- `Vec2f pan`

**Returns** — [Vec2f](../../utils/Math.hpp.md#vec2f)

Holds a pan offset inside the configured bounds. Each side is optional, so a canvas can be bounded on one edge and infinite on the others. The maximum is measured against the visible extent rather than the raw coordinate, so the far edge of the content stops at the far edge of the viewport instead of scrolling past it -- and because that extent depends on zoom, it is recomputed per axis here. When the bounded range is narrower than the viewport there is nothing to pan, so the offset pins to the minimum.

---

### setZoom

```cpp
setZoom(float z)
```

**Parameters**

- `float z`

**Returns** — void

Sets both axes to the same zoom.

---

### setZoom

```cpp
setZoom(float zx, float zy)
```

**Parameters**

- `float zx`
- `float zy`

**Returns** — void

Sets each axis independently, clamped to the configured range. The pan is re-clamped afterwards, since changing zoom changes how much of the canvas is visible and so how far it may travel.

---

### zoomAt

```cpp
zoomAt(Vec2f pivotWindowPx, float factor)
```

**Parameters**

- `Vec2f pivotWindowPx`
- `float factor`

**Returns** — void

Scales both axes by one factor about a pivot in window pixels, honouring the per-axis locks -- a locked axis is passed a factor of 1 and so is left where it is. Does nothing when zoom is disabled outright, or when both axes are locked.

---

### zoomAt

```cpp
zoomAt(Vec2f pivotWindowPx, float factorX, float factorY)
```

**Parameters**

- `Vec2f pivotWindowPx`
- `float factorX`
- `float factorY`

**Returns** — void

Scales each axis by its own factor about a pivot in window pixels, keeping the canvas point under that pivot visually fixed: the point is read at the old zoom and the pan rewritten so it lands back under the pivot at the new one. Ignores the axis locks, which is what makes it the programmatic entry point -- the gesture path goes through the single-factor overload.

---

### addChild

```cpp
addChild(Element* element, float x, float y)
```

**Parameters**

- `Element* element`
- `float x`
- `float y`

**Returns** — void

Adds a child and places it at a canvas-space position, snapped to the grid. Binds it to the owning [UILO](../../UILO.hpp.md#uilo) straight away when the canvas already has one, so an element added after the page is live is registered rather than left out of the element pool.

---

### setChildPosition

```cpp
setChildPosition(Element* element, float x, float y)
```

**Parameters**

- `Element* element`
- `float x`
- `float y`

**Returns** — void

Moves an existing child to a canvas-space position, snapped to the grid.

---

### getChildPosition

```cpp
getChildPosition(Element* element)
```

**Parameters**

- `Element* element`

**Returns** — [Vec2f](../../utils/Math.hpp.md#vec2f) -- canvas-space position, origin when the element is unknown

Where a child sits in canvas space.

---

### setPan

```cpp
setPan(Vec2f pan)
```

**Parameters**

- `Vec2f pan`

**Returns** — void

Moves the viewport to an absolute canvas-space offset, clamped to the configured bounds. Only marks the canvas dirty when the offset actually moved, so driving this every frame from a handler costs nothing while the view is still.

---

### update

```cpp
update(Rectf& parentBounds, float dt)
```

**Parameters**

- `Rectf& parentBounds`
- `float dt`

**Returns** — void

Resolves the canvas's own bounds, services middle-mouse panning, then places every child at its canvas-space position transformed by the pan and zoom. Zoom is applied in two parts: a uniform boost to [UILO](../../UILO.hpp.md#uilo)'s scale, so text, rounding and outlines grow with the view, and a per-axis size override applied after each child ticks, so a canvas zoomed differently on each axis stretches its children rather than only moving them. The scale boost is undone before returning, since it belongs to this subtree alone.

---

### render

```cpp
render()
```

**Returns** — void

Draws the backdrop, the grid, and then the children, with everything after the backdrop clipped to the canvas viewport so content panned past an edge disappears cleanly. The grid is drawn in canvas space stepped by the grid metric, so it travels and scales with the content rather than being painted on the window.

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

**Returns** — bool -- true when the canvas consumed the event

Single-axis scroll, forwarded to the two-axis path as a vertical delta so a plain wheel pans the canvas down.

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

**Returns** — bool -- true when the canvas consumed the event

Two-axis scroll, offered to the children first so a scrollable element sitting on the canvas keeps its own gesture. Otherwise the delta pans the view, divided by the zoom so a drag covers the same on-screen distance whatever the view is scaled to.

---

### checkZoom

```cpp
checkZoom(const Vec2f& mousePosition, float magnification)
```

**Parameters**

- `const Vec2f& mousePosition`
- `float magnification`

**Returns** — bool -- true when the canvas consumed the gesture

Pinch or Ctrl-scroll zoom about the pointer, offered to the children first so a nested zoomable element wins. `magnification` is an additive per-event ratio, so 0.05 means grow by five percent, and it is turned into a multiplier for zoomAt.
