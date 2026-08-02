# Knob.hpp

`include/elements/interactible/Knob.hpp`

[← index](../../README.md)

## Types

- [KnobValueChangedFuncPtr](#knobvaluechangedfuncptr)
- [KnobArcDir](#knobarcdir)
- [KnobOptions](#knoboptions)
- [Knob](#knob)

---

### KnobValueChangedFuncPtr

Storage signature for a knob's value callback, fired only when the value actually moves.

---

### KnobArcDir

Which way the arc sweeps from the start angle to the end angle. Together with those two angles this covers every knob layout: from 135 to 45 clockwise sweeps under the bottom, and the same pair counter-clockwise sweeps over the top.

---

### KnobOptions

Everything a [Knob](#knob) draws and the range it works over: the body and its rim, the unfilled track, the filled arc, the indicator line, the arc's geometry, and the value range and step. Colors come as a literal plus a role, where the role wins when it resolves against the active [Palette](../../Palette.hpp.md#palette) and the literal is the fallback.

> The body radius comes from the element's bounds rather than the options -- half the smaller side -- so a knob is sized like any other element and the thicknesses here are measured against that.

---

### Knob

A rotary value control. Dragging vertically changes the value, at a rate set by dragPixelsPerRange, which is the convention a mouse can actually work with -- following the pointer around the circle makes a small knob unusable. The wheel adjusts it too, accumulating sub-step motion so a stepped knob still responds to a slow trackpad, and a double click restores the configured default.

> sweepDegrees and angleForValue are pure functions of the options and the current value, exposed because an alternate renderer needs to lay the arc out exactly the way render() does.
