# Slider.hpp

`include/elements/interactible/Slider.hpp`

[← index](../../README.md)

## Types

- [ValueChangedFuncPtr](#valuechangedfuncptr)
- [ThumbShape](#thumbshape)
- [SliderOrientation](#sliderorientation)
- [SliderOptions](#slideroptions)
- [Slider](#slider)

## Functions

- [`getTrackRounding()`](#gettrackrounding)
- [`getThumbRounding()`](#getthumbrounding)

---

### ValueChangedFuncPtr

Storage signature for a slider's value callback. Fired whenever the value actually moves, not on every input event, so a handler can be expensive without being called on a frame where nothing changed.

---

### ThumbShape

How the slider's handle is drawn -- a circle, or a rectangle whose corner radius comes from setThumbRounding.

---

### SliderOrientation

Which axis the slider runs along. Horizontal fills left to right; Vertical fills bottom to top, so a level or volume control reads the way one is expected to.

---

### SliderOptions

Everything a [Slider](#slider) draws and the range it works over: the track, the filled portion, the thumb, the value range and step, and the orientation. Colors come as a literal plus a role, where the role wins when it resolves against the active [Palette](../../Palette.hpp.md#palette) and the literal is the fallback.

> setTrackThickness is read two ways. A value of 1 or below is a fraction of the cross-axis size, so the track scales with the element, and anything above 1 is a pixel thickness.

> A step of 0 is continuous; anything above snaps the value to that increment.

---

### Slider

A draggable value control. Clicking anywhere on the track jumps the value there and begins a drag, so a press and a drag are the same gesture; double-clicking restores the configured default. The wheel adjusts it too, accumulating sub-step motion so a stepped slider still responds to a slow trackpad rather than ignoring deltas too small to cross an increment.

> The thumb is inset from both ends by its own half-size, so its edge stops at the track's edge instead of hanging past it, and the usable travel is measured against that inset span.

---

### getTrackRounding

```cpp
getTrackRounding()
```

**Returns** — float

Corner radius of the track bar, resolved in three steps: the value this slider was given, then the active Theme's, then 0.

---

### getThumbRounding

```cpp
getThumbRounding()
```

**Returns** — float

Corner radius of a Rect thumb, resolved the same three ways. Ignored by a Circle thumb.
