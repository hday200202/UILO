# Knob.cpp

`include/elements/interactible/Knob.cpp`

[← index](../../README.md)

## Functions

- [`wrap360(float a)`](#wrap360)
- [`Knob(Modifier modifier, KnobOptions options, const std::string& name)`](#knob)
- [`sweepDegrees()`](#sweepdegrees)
- [`angleForValue(float v)`](#angleforvalue)
- [`update(Rectf& parentBounds, float dt)`](#update)
- [`render()`](#render)
- [`checkHover(const Vec2f& mousePosition)`](#checkhover)
- [`checkLeftClick(const Vec2f& mousePosition)`](#checkleftclick)
- [`checkScroll(const Vec2f& mousePosition, float delta, bool precise, bool momentum)`](#checkscroll)
- [`onDeactivate()`](#ondeactivate)
- [`setValue(float v)`](#setvalue)
- [`applyValue(float raw)`](#applyvalue)

---

### wrap360

```cpp
wrap360(float a)
```

**Parameters**

- `float a`

**Returns** — float -- the angle in [0, 360)

Normalises an angle in degrees, keeping the result positive so a negative input still lands in range.

---

### Knob

```cpp
Knob(Modifier modifier, KnobOptions options, const std::string& name)
```

**Parameters**

- `Modifier modifier`
- `KnobOptions options`
- `const std::string& name`

**Returns** — [Knob](Knob.hpp.md#knob)

Constructs a knob and seats it at its configured default, clamped into range, or at the minimum when no default was given.

---

### sweepDegrees

```cpp
sweepDegrees()
```

**Returns** — float -- signed total sweep, positive counter-clockwise

How far the arc travels from the start angle to the end angle along the configured direction. The sign carries the direction, so callers can interpolate without branching on it. A start and end that coincide would otherwise give a zero-length arc and draw nothing, so that case is taken as a full revolution instead -- which is what someone asking for 0 to 0 means.

---

### angleForValue

```cpp
angleForValue(float v)
```

**Parameters**

- `float v`

**Returns** — float -- cartesian angle in degrees

Where a value sits along the arc. A degenerate range pins to the start angle rather than dividing by zero.

---

### update

```cpp
update(Rectf& parentBounds, float dt)
```

**Parameters**

- `Rectf& parentBounds`
- `float dt`

**Returns** — void

Resolves the knob's bounds and tracks a vertical drag while one is running. Dragging up always turns the indicator clockwise, which means the value delta is flipped when the arc itself sweeps clockwise -- otherwise the same gesture would move the pointer in opposite directions on two knobs configured differently. The button state is polled so a release outside the window still ends the drag.

---

### render

```cpp
render()
```

**Returns** — void

Draws the body and its rim, the arc track with the filled portion over it, and the indicator. The body radius is half the smaller side less the room the arc and its gap need, so the ring sits around the body rather than spilling outside the element. Tessellation is adaptive -- roughly a segment per pixel of circumference, and no coarser than about half a degree on the arc -- so a large knob stays smooth without making a small one expensive, with the configured segment count acting as a floor.

---

### checkHover

```cpp
checkHover(const Vec2f& mousePosition)
```

**Parameters**

- `const Vec2f& mousePosition`

**Returns** — bool -- true when the pointer is over the knob

Asks for the vertical resize cursor while the pointer is inside, which is the axis a drag actually works on.

---

### checkLeftClick

```cpp
checkLeftClick(const Vec2f& mousePosition)
```

**Parameters**

- `const Vec2f& mousePosition`

**Returns** — bool -- true when the knob took the click

Begins a drag from the current value, or restores the configured default on a double click. Unlike a [Slider](Slider.hpp.md#slider) the value does not jump to the press: a knob has no position under the pointer to jump to.

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

**Returns** — bool -- true when the knob consumed the event

Adjusts the value by the wheel, accumulating the delta so a stepped knob still responds to deltas too small to cross an increment. Overscroll at either end is discarded rather than banked, so reversing direction moves the value immediately.

---

### onDeactivate

```cpp
onDeactivate()
```

**Returns** — void

Ends any drag when focus moves elsewhere.

---

### setValue

```cpp
setValue(float v)
```

**Parameters**

- `float v`

**Returns** — void

Sets the value programmatically, through the same clamping, snapping and change-callback path a drag uses.

---

### applyValue

```cpp
applyValue(float raw)
```

**Parameters**

- `float raw`

**Returns** — void

The single place the value changes: clamps into range, snaps to the step when one is set, and fires onValueChanged only when the result actually differs.
