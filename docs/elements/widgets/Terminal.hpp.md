# Terminal.hpp

`include/elements/widgets/Terminal.hpp`

[← index](../../README.md)

## Types

- [TerminalCell](#terminalcell)
- [TerminalOptions](#terminaloptions)
- [Terminal](#terminal)

## Functions

- [`setAnsiColor(int index, const Color& c)`](#setansicolor)
- [`getAnsiColor(int index)`](#getansicolor)
- [`getFontPath()`](#getfontpath)
- [`getCharSize()`](#getcharsize)
- [`getRounding()`](#getrounding)

---

### TerminalCell

One character cell: the codepoint it shows and the colours and style it is drawn with. Colours are resolved to literals when the cell is written rather than stored as palette roles, because a terminal's colours come from the program it is running, not from the theme.

---

### TerminalOptions

Everything a [Terminal](#terminal) draws and how it starts its shell: the font and cell metrics, the default colours, the sixteen-colour palette the escape sequences index into, the scrollback depth, and which shell to launch. Colours come as a literal plus a role like the rest of the library, but only for the widget's own surface -- the per-cell colours a program emits are always literals.

> A terminal has to be monospaced to lay out as a grid, so the font should be one. Cell size is measured from the font rather than configured, which is what keeps the grid aligned at any character size.

---

### Terminal

A terminal emulator: a real shell on a pseudo-terminal, its output parsed into a character grid and drawn, and keystrokes encoded back to it. Native only -- it needs a PTY and a process, neither of which the Wt backend has, so on the web it renders as an inert panel.

> The grid is sized from the font's own metrics and the element's bounds, so resizing the window reflows the shell: the new dimensions are pushed through to the PTY, which raises SIGWINCH and makes a running program redraw at the new size.

> Output is parsed by a small state machine covering what a shell and the common full-screen programs actually emit: SGR colour and style including 256-colour and true colour, cursor addressing, erase, insert and delete, scrolling regions, and the alternate screen. Anything unrecognised is swallowed rather than printed, so an unhandled sequence leaves no litter on screen.

> Scrollback is kept as whole rows pushed off the top. Scrolling the view only changes what is drawn; the shell is never told, because as far as it is concerned the screen is still the same size.

---

### setAnsiColor

```cpp
setAnsiColor(int index, const Color& c)
```

**Parameters**

- `int index`
- `const Color& c`

**Returns** — [TerminalOptions](#terminaloptions)&

Overrides one of the sixteen colours escape sequences index into. An index outside 0..15 is ignored rather than clamped, so a mistake does not silently recolour something else.

---

### getAnsiColor

```cpp
getAnsiColor(int index)
```

**Parameters**

- `int index`

**Returns** — [Color](../../utils/Color.hpp.md#color)

One of the sixteen indexed colours, or the default foreground for an index outside the range.

---

### getFontPath

```cpp
getFontPath()
```

**Returns** — const std::string&

[Font](../../renderer/Renderer.hpp.md#font) for the cell grid, falling back to the active Theme's. It should be monospaced; a proportional face still renders but the columns will not line up, since the grid is measured from one glyph's advance.

---

### getCharSize

```cpp
getCharSize()
```

**Returns** — unsigned int

Character size in unscaled pixels, resolved from this widget, then the Theme, then 14.

---

### getRounding

```cpp
getRounding()
```

**Returns** — float

Corner radius of the terminal's surface, resolved from this widget, then the Theme, then 0.
