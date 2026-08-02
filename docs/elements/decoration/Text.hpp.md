# Text.hpp

`include/elements/decoration/Text.hpp`

[← index](../../README.md)

## Types

- [TextOptions](#textoptions)
- [Text](#text)

## Functions

- [`getFontPath()`](#getfontpath)
- [`getCharSize()`](#getcharsize)

---

### TextOptions

Everything a [Text](#text) draws: the font, the string, the character size, the colour, the style flags, and where the glyphs sit inside the element's bounds. The colour comes as a literal plus a role, where the role wins when it resolves against the active [Palette](../../Palette.hpp.md#palette) and the literal is the fallback. Character size is left unset by default, which makes the element size its own text from its height.

> setTextAlignX / setTextAlignY place the glyphs within the element, which is separate from the [Modifier](../Modifier.hpp.md#modifier)'s align, which places the element within its parent.

---

### Text

A string drawn with a loaded font. The font is loaded on the first update, once the element is bound to a [UILO](../../UILO.hpp.md#uilo) and so has a renderer to load it with, which is why a Text built before the page is added still works. With no explicit char size the glyphs are sized from the element's height, so a Text in a fixed-height row scales with it. Optional word wrapping re-flows the string whenever the element's width changes. Layout metrics are cached and only re-measured when the string, size, scale or wrap width changes, so drawing does not walk the UTF-8 and the glyph table every frame.

> setString() writes to a live string separate from the options, so a runtime update is read through getString() rather than getOptions().getContent().

---

### getFontPath

```cpp
getFontPath()
```

**Returns** — const std::string&

[Font](../../renderer/Renderer.hpp.md#font) for the string, falling back to the active Theme's when this element was not given one. May be either a path or a name the [Resources](../../utils/Resources.hpp.md#resources) font registry knows; [Text](#text) resolves which at load time.

---

### getCharSize

```cpp
getCharSize()
```

**Returns** — unsigned int

Character size in unscaled pixels, resolved in three steps: the value this element was given, then the active Theme's, then 30. Ask hasCharSize() to tell a real setting from the fallback, which is what [Text](#text) does before deciding to size the glyphs from its own height instead.
