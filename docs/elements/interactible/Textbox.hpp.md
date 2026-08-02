# Textbox.hpp

`include/elements/interactible/Textbox.hpp`

[← index](../../README.md)

## Types

- [TextboxOptions](#textboxoptions)
- [Textbox](#textbox)

## Functions

- [`getFontPath()`](#getfontpath)
- [`getCharSize()`](#getcharsize)
- [`getRounding()`](#getrounding)

---

### TextboxOptions

Everything a [Textbox](#textbox) draws and how it behaves: the font and text appearance, the box itself, the focus outline, the placeholder, the caret, the selection, the line-number gutter, and the editing rules. Colors come as a literal plus a role, where the role wins when it resolves against the active [Palette](../../Palette.hpp.md#palette) and the literal is the fallback -- so setting a literal colour alone often does nothing until the role is cleared.

> Multiline and wrap together decide the shape of the control. A single-line box is a field, a multiline one an editor, and a wrapping multiline one that was given a pixel height grows with its content.

---

### Textbox

An editable text field or multiline editor. The text is held as UTF-32 so an index is a codepoint rather than a byte, which is what keeps cursor movement, selection and word jumps simple; UTF-8 is produced only at the renderer and API boundaries.

> Multiline plus wrap builds a soft-wrapped display string alongside the real one, with a table of where breaks were inserted so display and text indices map back and forth. Everything positional -- the caret, hit testing, selection rectangles -- goes through that mapping.

> Height works one of two ways. A pixel height is a starting size the box may grow past as content is added, publishing the new height back to its parent; a percent height means the parent owns the slot, so the box fills it and taller content scrolls. Growing a percent box would mean rewriting its declared height to pixels, which severs it from the layout and stops it following a window resize.

> Layout metrics are cached and rebuilt only when the text, size, scale or wrap width changes, so an idle frame costs no measurement.

---

### getFontPath

```cpp
getFontPath()
```

**Returns** — const std::string&

[Font](../../renderer/Renderer.hpp.md#font) for the text, falling back to the active Theme's when this box was not given one. May be a path or a name the [Resources](../../utils/Resources.hpp.md#resources) font registry knows.

---

### getCharSize

```cpp
getCharSize()
```

**Returns** — unsigned int

Character size in unscaled pixels, resolved in three steps: the value this box was given, then the active Theme's, then 18. Ask hasCharSize() to tell a real setting from the fallback, which is what the box does before deciding to size the text from its own height instead.

---

### getRounding

```cpp
getRounding()
```

**Returns** — float

Corner radius, resolved in three steps: the value this box was given, then the active Theme's, then 0.
