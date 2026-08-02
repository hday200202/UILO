# Text.cpp

`include/elements/decoration/Text.cpp`

[← index](../../README.md)

## Functions

- [`Text(Modifier modifier, TextOptions options, const std::string& name)`](#text)
- [`init()`](#init)
- [`wrapContent(float maxWidth)`](#wrapcontent)
- [`rebuildText()`](#rebuildtext)
- [`isLoaded()`](#isloaded)
- [`setString(const std::string& content)`](#setstring)
- [`update(Rectf& parentBounds, float dt)`](#update)
- [`render()`](#render)

---

### Text

```cpp
Text(Modifier modifier, TextOptions options, const std::string& name)
```

**Parameters**

- `Modifier modifier`
- `TextOptions options`
- `const std::string& name`

**Returns** — [Text](Text.hpp.md#text)

Constructs a text element from a modifier and its options, taking the initial string and character size from them. The font is not loaded here: there is no renderer until the element is bound to a [UILO](../../UILO.hpp.md#uilo), so that waits for the first update.

---

### init

```cpp
init()
```

**Returns** — void

Loads the font, once, on the first update that has a [UILO](../../UILO.hpp.md#uilo) to load it through. A registered font name -- [Resources](../../utils/Resources.hpp.md#resources)::fonts::default_, or anything the application added -- resolves to its path here, while a plain path is handed back unchanged, so setFont("assets/fonts/X.ttf") is unaffected. A failed load leaves the element unloaded, which makes it draw nothing rather than crash, and the next update tries again.

---

### wrapContent

```cpp
wrapContent(float maxWidth)
```

**Parameters**

- `float maxWidth`

**Returns** — std::string -- the string with newlines inserted

Greedy word wrap at a pixel width. Words are added to the current line until measuring one more would overflow, at which point the line is broken. A single word wider than maxWidth is left on its own line rather than split, since breaking mid-word reads worse than overflowing. Returns the string unchanged when there is no font loaded or no width to wrap against.

---

### rebuildText

```cpp
rebuildText()
```

**Returns** — void

Re-derives the string that is actually drawn, re-wrapping it when wrapping is on and a width is known, and invalidates the cached layout metrics so the next draw measures afresh.

---

### isLoaded

```cpp
isLoaded()
```

**Returns** — bool

Whether the font loaded. False until the first update after the element is bound to a [UILO](../../UILO.hpp.md#uilo), and permanently false when the font could not be read.

---

### setString

```cpp
setString(const std::string& content)
```

**Parameters**

- `const std::string& content`

**Returns** — void

Replaces the string being drawn. Setting the same string is a no-op, so calling this every frame from a handler does not force a redraw or a re-measure. Re-wraps immediately when the font is loaded; otherwise the first update does it.

---

### update

```cpp
update(Rectf& parentBounds, float dt)
```

**Parameters**

- `Rectf& parentBounds`
- `float dt`

**Returns** — void

Loads the font if that has not happened yet, resolves the element's bounds, and re-derives the drawn string when anything it depends on has moved. With no explicit character size the glyphs are sized from the element's own height, so a [Text](Text.hpp.md#text) in a row grows with it; a wrapping [Text](Text.hpp.md#text) re-flows when its width changes; and any change of UI scale invalidates the measurements either way. Returns early while the font is still unloaded, which is what makes a missing font draw nothing instead of crashing.

---

### render

```cpp
render()
```

**Returns** — void

Draws the string, placed inside the element's bounds according to the options' text alignment. Metrics are measured only when the cache is stale, so an unchanged string costs no UTF-8 or glyph-table walk per frame. The colour resolves through the [Palette](../../Palette.hpp.md#palette), falling back to the literal when there is no [UILO](../../UILO.hpp.md#uilo).
