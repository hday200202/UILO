# Textbox.cpp

`include/elements/interactible/Textbox.cpp`

[← index](../../README.md)

## Functions

- [`u32ToUtf8(const std::u32string& s)`](#u32toutf8)
- [`utf8ToU32(const std::string& s)`](#utf8tou32)
- [`isWordChar(char32_t c)`](#iswordchar)
- [`shouldWrap(const TextboxOptions& opts)`](#shouldwrap)
- [`autoGrows()`](#autogrows)
- [`maxScrollY(int lineCount, float lh)`](#maxscrolly)
- [`resolvedFontPath()`](#resolvedfontpath)
- [`gutterWidth()`](#gutterwidth)
- [`textArea()`](#textarea)
- [`gutterArea()`](#gutterarea)
- [`recomputeGutterWidth(Renderer& renderer, float pxH)`](#recomputegutterwidth)
- [`logicalLineOfCursor()`](#logicallineofcursor)
- [`lineHeight()`](#lineheight)
- [`displayText()`](#displaytext)
- [`charScreenPos(size_t idx)`](#charscreenpos)
- [`hitTestChar(Vec2f screenPos)`](#hittestchar)
- [`rebuildSfText()`](#rebuildsftext)
- [`rebuildWrapped()`](#rebuildwrapped)
- [`computeTextOrigin()`](#computetextorigin)
- [`ensureCursorVisible()`](#ensurecursorvisible)
- [`textToDisplay(size_t textIdx)`](#texttodisplay)
- [`displayToText(size_t dispIdx)`](#displaytotext)
- [`hasSelection()`](#hasselection)
- [`deleteSelection()`](#deleteselection)
- [`resetBlink()`](#resetblink)
- [`lineStart(size_t pos)`](#linestart)
- [`lineEnd(size_t pos)`](#lineend)
- [`wordLeft(size_t pos)`](#wordleft)
- [`wordRight(size_t pos)`](#wordright)
- [`getString()`](#getstring)
- [`setString(const std::string& s)`](#setstring)
- [`update(Rectf& parentBounds, float dt)`](#update)
- [`render()`](#render)
- [`renderLineNumbers(Renderer& renderer, float pxH)`](#renderlinenumbers)
- [`checkLeftClick(const Vec2f& mousePos)`](#checkleftclick)
- [`checkHover(const Vec2f& mousePos)`](#checkhover)
- [`onDeactivate()`](#ondeactivate)
- [`checkScroll(const Vec2f& mousePos, float delta, bool precise, bool momentum)`](#checkscroll)
- [`handleTextInput(char32_t c)`](#handletextinput)
- [`insertTab()`](#inserttab)
- [`handleKeyInput(SDL_Keycode key, bool shift, bool ctrl, bool gui)`](#handlekeyinput)

---

### u32ToUtf8

```cpp
u32ToUtf8(const std::u32string& s)
```

**Parameters**

- `const std::u32string& s`

**Returns** — std::string

Encodes UTF-32 to UTF-8. The text is held as UTF-32 so an index is a codepoint rather than a byte, which is what makes cursor movement and selection arithmetic simple; the renderer takes UTF-8, so a conversion happens at the boundary.

---

### utf8ToU32

```cpp
utf8ToU32(const std::string& s)
```

**Parameters**

- `const std::string& s`

**Returns** — std::u32string

Decodes UTF-8 to UTF-32. A truncated sequence at the end of the input is skipped rather than producing a partial codepoint.

---

### isWordChar

```cpp
isWordChar(char32_t c)
```

**Parameters**

- `char32_t c`

**Returns** — bool

Whether a codepoint counts as part of a word for the ctrl-arrow jumps. Everything above ASCII is treated as a word character, so accented and non-Latin text is not split into fragments.

---

### shouldWrap

```cpp
shouldWrap(const TextboxOptions& opts)
```

**Parameters**

- `const TextboxOptions& opts`

**Returns** — bool

Whether soft wrapping is active. Password mode never wraps, since the masked string has no words to break on.

---

### autoGrows

```cpp
autoGrows()
```

**Returns** — bool

Whether the box grows with its content instead of filling its slot.

---

### maxScrollY

```cpp
maxScrollY(int lineCount, float lh)
```

**Parameters**

- `int lineCount`
- `float lh`

**Returns** — float

How far the view may scroll vertically. A growing box only has something to scroll once maxResizeLines has capped it; a fill box scrolls whatever does not fit.

---

### resolvedFontPath

```cpp
resolvedFontPath()
```

**Returns** — std::string

The font to load, resolved the same way [Text](../decoration/Text.hpp.md#text) resolves one: a registered name becomes its path, a plain path is handed back unchanged.

---

### gutterWidth

```cpp
gutterWidth()
```

**Returns** — float

Width of the line-number gutter, 0 when it is off or the box is single line. Reads a cached value, since the real width needs the font.

---

### textArea

```cpp
textArea()
```

**Returns** — [Rectf](../../utils/Math.hpp.md#rectf)

The rectangle the text actually occupies: the bounds less the padding and the gutter. Everything else -- the caret, hit testing, wrap width, scrolling -- is measured against this, so insetting it here is all the gutter needs to be accounted for everywhere.

---

### gutterArea

```cpp
gutterArea()
```

**Returns** — [Rectf](../../utils/Math.hpp.md#rectf)

The strip the line numbers are drawn in, to the left of the text area.

---

### recomputeGutterWidth

```cpp
recomputeGutterWidth(Renderer& renderer, float pxH)
```

**Parameters**

- `Renderer& renderer`
- `float pxH`

**Returns** — void

Measures the gutter from the widest line number it will have to show, at the line-number character size, plus padding and whatever slack bold needs. Cached because textArea() is const and called from everywhere, and the measurement needs a renderer.

---

### logicalLineOfCursor

```cpp
logicalLineOfCursor()
```

**Returns** — size_t

Which logical line the cursor is on, counting hard newlines only, so a soft-wrapped line counts once.

---

### lineHeight

```cpp
lineHeight()
```

**Returns** — float

Height of one line. Uses the font's real ascent, descent and gap once the font has loaded, and falls back to an estimate from the character size before then.

---

### displayText

```cpp
displayText()
```

**Returns** — std::u32string

The text as it should appear, which is a run of asterisks in password mode and the text itself otherwise.

---

### charScreenPos

```cpp
charScreenPos(size_t idx)
```

**Parameters**

- `size_t idx`

**Returns** — [Vec2f](../../utils/Math.hpp.md#vec2f)

Where a text index sits on screen. The index is mapped through the soft wrap table first when wrapping is on, since inserted breaks make display and text indices diverge.

---

### hitTestChar

```cpp
hitTestChar(Vec2f screenPos)
```

**Parameters**

- `Vec2f screenPos`

**Returns** — size_t

The text index nearest a screen position, used to place the cursor on a click or drag. Resolved in two passes -- first the visual line, then the closest boundary along it -- so clicking past the end of a short line lands at that line's end rather than on whatever character happens to be nearest in a straight line.

---

### rebuildSfText

```cpp
rebuildSfText()
```

**Returns** — void

Rebuilds everything derived from the text: the line height, the gutter width, the wrapped display string and the per-character positions. The gutter is measured before wrapping because it takes width off the text area and so changes where lines break.

---

### rebuildWrapped

```cpp
rebuildWrapped()
```

**Returns** — void

Builds the display string with soft breaks inserted, and records the text indices they were inserted at so display and text positions can be mapped back and forth. Breaks on word boundaries where it can and mid-word when a single word is wider than the line.

---

### computeTextOrigin

```cpp
computeTextOrigin()
```

**Returns** — void

Works out where the text block is drawn, from the alignment and the scroll offset. A growing box asking to be centred is the auto-grow case, so it centres against the height it was built at rather than the height it has now -- otherwise its first line would drift down as lines were added.

---

### ensureCursorVisible

```cpp
ensureCursorVisible()
```

**Returns** — void

Scrolls just enough to bring the cursor inside the text area, on whichever axis it left. Called only when the cursor actually moved, so typing does not fight a user's own scrolling.

---

### textToDisplay

```cpp
textToDisplay(size_t textIdx)
```

**Parameters**

- `size_t textIdx`

**Returns** — size_t

Maps a text index to its position in the wrapped display string, adding one for each soft break inserted before it.

---

### displayToText

```cpp
displayToText(size_t dispIdx)
```

**Parameters**

- `size_t dispIdx`

**Returns** — size_t

The inverse mapping, from a display index back to the text.

---

### hasSelection

```cpp
hasSelection()
```

**Returns** — bool

Whether anything is selected, which is simply the cursor and the anchor sitting at different positions.

---

### deleteSelection

```cpp
deleteSelection()
```

**Returns** — void

Removes the selected range and collapses the cursor to where it began.

---

### resetBlink

```cpp
resetBlink()
```

**Returns** — void

Shows the caret and restarts its blink timer, so it is solid at the moment of any edit or cursor move rather than possibly mid- blink.

---

### lineStart

```cpp
lineStart(size_t pos)
```

**Parameters**

- `size_t pos`

**Returns** — size_t

Index of the first character on the line containing a position.

---

### lineEnd

```cpp
lineEnd(size_t pos)
```

**Parameters**

- `size_t pos`

**Returns** — size_t

Index just past the last character on the line containing a position.

---

### wordLeft

```cpp
wordLeft(size_t pos)
```

**Parameters**

- `size_t pos`

**Returns** — size_t

The next word boundary to the left, skipping any run of separators first so a ctrl-left from after a space lands at the start of the previous word rather than on the space.

---

### wordRight

```cpp
wordRight(size_t pos)
```

**Parameters**

- `size_t pos`

**Returns** — size_t

The next word boundary to the right, by the mirror of the same rule.

---

### getString

```cpp
getString()
```

**Returns** — std::string

The current text as UTF-8. This is the real text, not the masked or soft-wrapped display form.

---

### setString

```cpp
setString(const std::string& s)
```

**Parameters**

- `const std::string& s`

**Returns** — void

Replaces the text and clamps the cursor and anchor into the new length, so a shorter string cannot leave them dangling past the end.

---

### update

```cpp
update(Rectf& parentBounds, float dt)
```

**Parameters**

- `Rectf& parentBounds`
- `float dt`

**Returns** — void

Resolves bounds, rebuilds the text when anything it depends on has moved, sizes the box, advances the caret blink and services a drag selection. Height is handled one of two ways: a pixel- sized box grows with its content and publishes that height back to its parent, while a percent-sized one fills the slot it was given and scrolls instead -- growing the latter would mean rewriting its declared height to pixels, which would sever it from the layout and stop it tracking a window resize.

---

### render

```cpp
render()
```

**Returns** — void

Draws the background and focus outline, the line-number gutter, then the selection, the text and the caret, all clipped to the text area. The placeholder replaces the text when the box is empty and unfocused. The caret is clamped so its full width stays inside the area: centred on the insertion point it would otherwise be sliced in half at the start of every line.

---

### renderLineNumbers

```cpp
renderLineNumbers(Renderer& renderer, float pxH)
```

**Parameters**

- `Renderer& renderer`
- `float pxH`

**Returns** — void

Draws the gutter and one number per logical line, so a soft- wrapped line is numbered once and its continuation rows are left blank. Numbers may be smaller than the body text, in which case they are baseline-aligned with it rather than sitting at the top of the line box. The cursor's line can take its own colour and style.

---

### checkLeftClick

```cpp
checkLeftClick(const Vec2f& mousePos)
```

**Parameters**

- `const Vec2f& mousePos`

**Returns** — bool -- true when the click landed on the box

Takes focus and places the cursor where the click landed, extending the selection instead when shift is held. Also arms a drag, so a press and sweep selects a range.

---

### checkHover

```cpp
checkHover(const Vec2f& mousePos)
```

**Parameters**

- `const Vec2f& mousePos`

**Returns** — bool -- true when the pointer is over the box

Asks for the text cursor while the pointer is inside.

---

### onDeactivate

```cpp
onDeactivate()
```

**Returns** — void

Releases focus when something else is clicked, ending any drag and hiding the caret.

---

### checkScroll

```cpp
checkScroll(const Vec2f& mousePos, float delta, bool precise, bool momentum)
```

**Parameters**

- `const Vec2f& mousePos`
- `float delta`
- `bool precise`
- `bool momentum`

**Returns** — bool -- true when the box consumed the event

Scrolls a multiline box, converting the delta exactly the way a scrollable [Column](../containers/Column.hpp.md#column) does so the two feel identical under one gesture. The offset is continuous rather than snapped to whole lines, which is what lets a trackpad's momentum tail read as smooth instead of stepping. Declines when nothing overflows, so a wheel over a short editor scrolls the page behind it instead.

---

### handleTextInput

```cpp
handleTextInput(char32_t c)
```

**Parameters**

- `char32_t c`

**Returns** — void

Inserts a typed codepoint, replacing the selection first. Control characters are ignored here; the ones that mean something arrive through handleKeyInput instead.

---

### insertTab

```cpp
insertTab()
```

**Returns** — void

Inserts one indent as spaces. Tab always inserts spaces rather than a literal tab character, because the renderer has no tab- advance logic and a real one would draw as a missing-glyph box. Ignored on a single-line box, where indenting means nothing.

---

### handleKeyInput

```cpp
handleKeyInput(SDL_Keycode key, bool shift, bool ctrl, bool gui)
```

**Parameters**

- `SDL_Keycode key`
- `bool shift`
- `bool ctrl`
- `bool gui`

**Returns** — void

Handles every key that is not plain typing: cursor movement with the arrows, Home and End, word jumps with ctrl, deletion, Tab, Enter, Escape, and the clipboard and select-all shortcuts. Shift extends the selection by leaving the anchor where it is; without it the anchor follows the cursor and the selection collapses.

> Word jumps stay on Control alone. Only the clipboard and select-all shortcuts accept Command as well, which is what a Mac user expects.
