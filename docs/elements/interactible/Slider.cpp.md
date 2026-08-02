# Slider.cpp

`include/elements/interactible/Slider.cpp`

[← index](../../README.md)

## Functions

- [`Slider(Modifier modifier, SliderOptions options, const std::string& name)`](#slider)
- [`update(Rectf& parentBounds, float dt)`](#update)
- [`render()`](#render)
- [`checkHover(const Vec2f& mousePosition)`](#checkhover)
- [`checkLeftClick(const Vec2f& mousePosition)`](#checkleftclick)
- [`checkScroll(const Vec2f& mousePosition, float delta, bool precise, bool momentum)`](#checkscroll)
- [`onDeactivate()`](#ondeactivate)
- [`setValue(float value)`](#setvalue)
- [`valueFromMouseX(float mouseX)`](#valuefrommousex)
- [`applyValue(float raw)`](#applyvalue)
- [`resolveThumbHalfWidth()`](#resolvethumbhalfwidth)
- [`resolveThumbHalfHeight()`](#resolvethumbhalfheight)
- [`valueFromMouseY(float mouseY)`](#valuefrommousey)

---

### Slider

```cpp
Slider(Modifier modifier, SliderOptions options, const std::string& name)
```

**Parameters**

- `Modifier modifier`
- `SliderOptions options`
- `const std::string& name`

**Returns** — [Slider](Slider.hpp.md#slider)

Constructs a slider and seats it at its configured default, clamped into range, or at the minimum when no default was given.

---

### update

```cpp
update(Rectf& parentBounds, float dt)
```

**Parameters**

- `Rectf& parentBounds`
- `float dt`

**Returns** — void

Resolves the slider's bounds and, while a drag is running, tracks the pointer. The button state is polled rather than driven by an event so a release outside the window still ends the drag, and the resize cursor is re-requested each frame because [UILO](../../UILO.hpp.md#uilo) clears the request pool at the top of every one.

---

### render

```cpp
render()
```

**Returns** — void

Draws the track, the filled portion up to the thumb, and the thumb itself, in whichever orientation the options ask for. A vertical slider fills from the bottom up, which is the direction a level control is read in. The track's thickness is a fraction of the cross axis when it is 1 or below and a pixel count otherwise, so the same options work at any element size.

---

### checkHover

```cpp
checkHover(const Vec2f& mousePosition)
```

**Parameters**

- `const Vec2f& mousePosition`

**Returns** — bool -- true when the pointer is over the slider

Asks for the resize cursor along the slider's own axis while the pointer is inside, so the control reads as draggable.

---

### checkLeftClick

```cpp
checkLeftClick(const Vec2f& mousePosition)
```

**Parameters**

- `const Vec2f& mousePosition`

**Returns** — bool -- true when the slider took the click

Jumps the value to wherever the track was clicked and starts a drag, so pressing and dragging are one gesture rather than requiring the thumb to be grabbed exactly. A double click within the usual window restores the configured default instead.

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

**Returns** — bool -- true when the slider consumed the event

Adjusts the value by the wheel, accumulating the delta so a stepped slider still responds to a trackpad whose individual deltas are too small to cross an increment. Overscroll at either end is discarded rather than banked, so reversing direction moves the value immediately instead of first unwinding hidden accumulation.

---

### onDeactivate

```cpp
onDeactivate()
```

**Returns** — void

Ends any drag when focus moves elsewhere, so the thumb cannot stay latched to the pointer after a click lands on something else.

---

### setValue

```cpp
setValue(float value)
```

**Parameters**

- `float value`

**Returns** — void

Sets the value programmatically, through the same clamping, snapping and change-callback path a drag uses.

---

### valueFromMouseX

```cpp
valueFromMouseX(float mouseX)
```

**Parameters**

- `float mouseX`

**Returns** — float -- the value that x position represents

Maps a window x onto the slider's range. The travel is measured between the thumb's two extreme centres rather than the full width, so dragging to either end puts the thumb's edge flush with the track's rather than hanging past it.

---

### applyValue

```cpp
applyValue(float raw)
```

**Parameters**

- `float raw`

**Returns** — void

The single place the value changes: clamps into range, snaps to the step when one is set, and fires onValueChanged only when the result actually differs. That guard is what lets a drag call this every frame without flooding a handler.

---

### resolveThumbHalfWidth

```cpp
resolveThumbHalfWidth()
```

**Returns** — float

Half the thumb's drawn width in render pixels. A circular thumb with no size set falls back to a fraction of the element's cross axis, so a slider given only a height still gets a proportionate thumb.

---

### resolveThumbHalfHeight

```cpp
resolveThumbHalfHeight()
```

**Returns** — float

Half the thumb's drawn height, resolved the same way as the width, against the other axis.

---

### valueFromMouseY

```cpp
valueFromMouseY(float mouseY)
```

**Parameters**

- `float mouseY`

**Returns** — float -- the value that y position represents

Maps a window y onto the range for a vertical slider, inverted so the top of the track is the maximum.
