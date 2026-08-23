# Spacer.hpp

`include/elements/decoration/Spacer.hpp`

[← index](../../README.md)

## Types

- [SpacerOptions](#spaceroptions)
- [Spacer](#spacer)

## Functions

- [`getRounding()`](#getrounding)

---

### SpacerOptions

Everything a [Spacer](#spacer) draws, which by default is nothing: the fill is transparent and there is no border, so a spacer is pure empty space. Giving it a colour turns it into a bar or a divider, and giving it an outline alone turns it into a frame. Colors come as a literal plus a role, where the role wins when it resolves against the active [Palette](../../Palette.hpp.md#palette) and the literal is the fallback.

---

### Spacer

Empty space in a layout, and the usual way to push siblings apart or hold a gap open. Takes part in layout like any other element -- a percent width shares the space left over, a pixel width is fixed -- but draws nothing unless it was given a fill or an outline. Carries no callbacks of its own, so it never swallows a click or hover meant for the container it sits in.

---

### getRounding

```cpp
getRounding()
```

**Returns** — float

Corner radius, resolved in three steps: the value this element was given, then the active [Theme](../../utils/Theme.hpp.md#theme)'s, then 0. Resolved on every read rather than cached, so changing the [Theme](../../utils/Theme.hpp.md#theme) restyles a spacer already on screen.

> Outline thickness is deliberately left unthemed, because a [Spacer](#spacer) is a gap and a themed border would draw a line around every one of them.
