# Terminal.cpp

`include/elements/widgets/Terminal.cpp`

[← index](../../README.md)

## Functions

- [`utf8Encode(char32_t cp)`](#utf8encode)
- [`csiParams(const std::string& s, std::vector&lt;int&gt;& out)`](#csiparams)
- [`param(const std::vector&lt;int&gt;& p, size_t i, int dflt)`](#param)
- [`Terminal(Modifier modifier, TerminalOptions options, const std::string& name)`](#terminal)
- [`~Terminal()`](#terminal)
- [`setOptions(const TerminalOptions& opts)`](#setoptions)
- [`start()`](#start)
- [`stop()`](#stop)
- [`isRunning()`](#isrunning)
- [`send(const std::string& text)`](#send)
- [`cell(int col, int row)`](#cell)
- [`resizeGrid(int cols, int rows)`](#resizegrid)
- [`clearRegion(int fromCol, int fromRow, int toCol, int toRow)`](#clearregion)
- [`scrollUp(int lines)`](#scrollup)
- [`scrollDown(int lines)`](#scrolldown)
- [`newline()`](#newline)
- [`putChar(char32_t c)`](#putchar)
- [`useAlternateScreen(bool on)`](#usealternatescreen)
- [`indexedColor(int n)`](#indexedcolor)
- [`applySgr()`](#applysgr)
- [`handleCsi(char final)`](#handlecsi)
- [`handleOsc()`](#handleosc)
- [`handleControl(char c)`](#handlecontrol)
- [`feed(const std::string& bytes)`](#feed)
- [`contentArea()`](#contentarea)
- [`gridTopY()`](#gridtopy)
- [`remeasure()`](#remeasure)
- [`update(Rectf& parentBounds, float dt)`](#update)
- [`render()`](#render)
- [`screenText()`](#screentext)
- [`viewText()`](#viewtext)
- [`scrollToBottom()`](#scrolltobottom)
- [`lineAt(long absolute, int& len)`](#lineat)
- [`hitTestLine(const Vec2f& pos)`](#hittestline)
- [`hitTestColumn(const Vec2f& pos)`](#hittestcolumn)
- [`normalizedSelection(long& fromLine, int& fromCol, long& toLine, int& toCol)`](#normalizedselection)
- [`selectedText()`](#selectedtext)
- [`clearSelection()`](#clearselection)
- [`selectAll()`](#selectall)
- [`selectRange(long fromLine, int fromCol, long toLine, int toCol)`](#selectrange)
- [`copySelection()`](#copyselection)
- [`pasteClipboard()`](#pasteclipboard)
- [`checkLeftClick(const Vec2f& mousePos)`](#checkleftclick)
- [`checkHover(const Vec2f& mousePos)`](#checkhover)
- [`checkScroll(const Vec2f& mousePos, float delta, bool precise, bool momentum)`](#checkscroll)
- [`onDeactivate()`](#ondeactivate)
- [`handleTextInput(char32_t unicode)`](#handletextinput)
- [`sendKey(SDL_Keycode key, bool shift, bool ctrl)`](#sendkey)
- [`handleKeyInput(SDL_Keycode key, bool shift, bool ctrl)`](#handlekeyinput)

---

### utf8Encode

```cpp
utf8Encode(char32_t cp)
```

**Parameters**

- `char32_t cp`

**Returns** — std::string

Encodes one codepoint, for handing a cell's character back to the renderer, which takes UTF-8.

---

### csiParams

```cpp
csiParams(const std::string& s, std::vector<int>& out)
```

**Parameters**

- `const std::string& s`
- `std::vector<int>& out`

**Returns** — void

Splits a CSI parameter string on semicolons into numbers. An omitted parameter becomes -1 rather than 0, because the two mean different things: most sequences read a missing parameter as 1, and conflating it with an explicit 0 breaks them.

---

### param

```cpp
param(const std::vector<int>& p, size_t i, int dflt)
```

**Parameters**

- `const std::vector<int>& p`
- `size_t i`
- `int dflt`

**Returns** — int

One CSI parameter, substituting the default for a missing or omitted one.

---

### Terminal

```cpp
Terminal(Modifier modifier, TerminalOptions options, const std::string& name)
```

**Parameters**

- `Modifier modifier`
- `TerminalOptions options`
- `const std::string& name`

**Returns** — [Terminal](Terminal.hpp.md#terminal)

Constructs the widget and its initial grid. The shell is not started here: the grid has to be measured against real bounds first, so it starts on the first update instead.

---

### ~Terminal

```cpp
~Terminal()
```

Stops the shell, so closing a page never leaves one running.

---

### setOptions

```cpp
setOptions(const TerminalOptions& opts)
```

**Parameters**

- `const TerminalOptions& opts`

**Returns** — void

Replaces the options and forces the cell metrics to be measured again, since the font or character size may have moved.

---

### start

```cpp
start()
```

**Returns** — bool -- false when no shell could be started

Launches the shell on a pseudo-terminal sized to the current grid. Starting twice is harmless.

---

### stop

```cpp
stop()
```

**Returns** — void

Ends the shell. The widget keeps whatever is on screen and can be started again.

---

### isRunning

```cpp
isRunning()
```

**Returns** — bool

Whether a shell is attached and has not exited.

---

### send

```cpp
send(const std::string& text)
```

**Parameters**

- `const std::string& text`

**Returns** — void

Feeds text to the shell as though it had been typed, which is also how a caller runs a command programmatically -- append a newline to execute it.

---

### cell

```cpp
cell(int col, int row)
```

**Parameters**

- `int col`
- `int row`

**Returns** — [TerminalCell](Terminal.hpp.md#terminalcell)&

One cell of the active grid, clamped into range so a malformed escape sequence cannot address outside it.

---

### resizeGrid

```cpp
resizeGrid(int cols, int rows)
```

**Parameters**

- `int cols`
- `int rows`

**Returns** — void

Reshapes the grid, keeping the written content at the top where the shell put it. A resize spends the blank rows below the content first, so growing and shrinking normally move nothing at all; only when the written rows themselves no longer fit does the top scroll away into history.

> Absolute line numbering is kept in step with any such shift, so a selection stays on the text it was made on.

> History is never pulled back onto the screen. Doing that on a grow moves text the shell believes it has already drawn, and the shell will overwrite the wrong rows the next time it paints.

> Lines are not re-wrapped. Reflowing needs the original unwrapped text, and a cell grid has already thrown that away.

---

### clearRegion

```cpp
clearRegion(int fromCol, int fromRow, int toCol, int toRow)
```

**Parameters**

- `int fromCol`
- `int fromRow`
- `int toCol`
- `int toRow`

**Returns** — void

Blanks an inclusive span, treated as a linear run of cells rather than a rectangle, because that is what erase-to-end-of-screen means. Cleared cells take the current background so an erase inside a coloured region stays coloured.

---

### scrollUp

```cpp
scrollUp(int lines)
```

**Parameters**

- `int lines`

**Returns** — void

Moves the scrolling region up, which is what a newline at the bottom does. Rows leaving the top are pushed to scrollback, but only when the region is the whole screen and the alternate screen is not in use -- a program scrolling a sub-region, or running full-screen, is not producing history.

---

### scrollDown

```cpp
scrollDown(int lines)
```

**Parameters**

- `int lines`

**Returns** — void

Moves the scrolling region down, opening blank rows at the top. Nothing is taken from scrollback: this is a program scrolling its own content, not the user looking back through history.

---

### newline

```cpp
newline()
```

**Returns** — void

Moves the cursor down a row, scrolling the region when it is already on the last one.

---

### putChar

```cpp
putChar(char32_t c)
```

**Parameters**

- `char32_t c`

**Returns** — void

Writes one character at the cursor and advances it. Wrapping is deferred: running past the last column arms a pending wrap rather than moving immediately, so a character landing exactly in the final column does not push the cursor onto the next line until something more is actually written. That is what stops a full-width line from leaving a stray blank row.

---

### useAlternateScreen

```cpp
useAlternateScreen(bool on)
```

**Parameters**

- `bool on`

**Returns** — void

Swaps between the normal screen and the alternate one full-screen programs draw on. The two are kept side by side and exchanged, so leaving vim puts back exactly what the shell had on screen before, and nothing the program drew reaches scrollback.

---

### indexedColor

```cpp
indexedColor(int n)
```

**Parameters**

- `int n`

**Returns** — [Color](../../utils/Color.hpp.md#color)

Resolves one of the 256 palette indices. 0-15 come from the configured sixteen; 16-231 are the 6x6x6 colour cube; 232-255 are the greyscale ramp. The cube and ramp are computed rather than stored, since they are defined by formula.

---

### applySgr

```cpp
applySgr()
```

**Returns** — void

Applies a select-graphic-rendition sequence to the pen: reset, the style flags and their individual offs, the eight normal and eight bright colours for both foreground and background, and the extended forms -- 5 for a 256-colour index and 2 for true colour. An unrecognised parameter is skipped rather than aborting the sequence, so one unknown attribute does not discard the rest.

---

### handleCsi

```cpp
handleCsi(char final)
```

**Parameters**

- `char final`

**Returns** — void

Dispatches a completed control sequence: cursor movement and addressing, erase in line and display, insert and delete of characters and lines, scrolling, the scrolling region, save and restore of the cursor, and the private mode toggles for cursor visibility, application cursor keys and the alternate screen. Mouse-reporting and bracketed-paste modes are recognised and ignored, which is deliberate: acknowledging them silently is better than letting the program think they are unsupported and fall back to something stranger.

---

### handleOsc

```cpp
handleOsc()
```

**Returns** — void

Operating-system commands, which are how a shell sets the window title and reports the working directory. Nothing here needs to affect the grid, so the sequence is consumed and discarded -- which is the point: without consuming it the payload would be printed as literal text.

---

### handleControl

```cpp
handleControl(char c)
```

**Parameters**

- `char c`

**Returns** — void

The C0 control characters that move the cursor rather than printing: carriage return, line feed, backspace, tab to the next eight-column stop, and bell. Bell is swallowed rather than flashing anything, since a UI toolkit has no business making noise on a program's behalf.

---

### feed

```cpp
feed(const std::string& bytes)
```

**Parameters**

- `const std::string& bytes`

**Returns** — void

Runs the parser over a chunk of shell output. The state machine has four states -- ground, after an escape, inside a control sequence, inside an operating-system command -- which is enough for everything a shell and the common full-screen programs emit. A multi-byte character split across two reads is held in a buffer and completed on the next one, so a chunk boundary in the middle of a codepoint never corrupts it.

---

### contentArea

```cpp
contentArea()
```

**Returns** — [Rectf](../../utils/Math.hpp.md#rectf)

The rectangle the grid is drawn in: the bounds less the padding.

---

### gridTopY

```cpp
gridTopY()
```

**Returns** — float

The y of the grid's first row. A whole number of rows almost never divides the height exactly, and that leftover has to go somewhere; it goes above the grid, so the last row always ends exactly one padding above the bottom edge. Left at the bottom it reads as the text being cut off short, because it sits right where the next line would have been.

---

### remeasure

```cpp
remeasure()
```

**Returns** — void

Works out the cell size from the font and, from that and the current bounds, how many columns and rows fit. The width comes from a single glyph's advance, which is why the font has to be monospaced. When the grid changes shape the PTY is told, so the shell reflows to match.

---

### update

```cpp
update(Rectf& parentBounds, float dt)
```

**Parameters**

- `Rectf& parentBounds`
- `float dt`

**Returns** — void

Resolves bounds, re-measures the grid, starts the shell on the first pass when autoStart is on, drains whatever the shell has written and parses it, and advances the cursor blink. Draining happens here rather than on a thread so the grid is only ever touched from one place, which is what keeps it safe to read from render without any locking.

---

### render

```cpp
render()
```

**Returns** — void

Draws the surface, then the visible rows, then the cursor, clipped to the widget. Background cells are emitted as merged runs rather than one rect per cell, and text likewise as runs of identical style, so a full screen costs a few dozen draws instead of a few thousand. Rows come from scrollback first when the view is scrolled back.

---

### screenText

```cpp
screenText()
```

**Returns** — std::string

Everything on the live screen as text, rows joined by newlines with trailing blanks trimmed. Intended for tests and for a "copy everything" action.

---

### viewText

```cpp
viewText()
```

**Returns** — std::string

The rows currently on display, which is the live screen when scrolled to the bottom and history when scrolled up. Rows that have aged out of the scrollback come back blank rather than shifting the rest.

---

### scrollToBottom

```cpp
scrollToBottom()
```

**Returns** — void

Returns the view to the live screen.

---

### lineAt

```cpp
lineAt(long absolute, int& len)
```

**Parameters**

- `long absolute`
- `int& len`

**Returns** — const [TerminalCell](Terminal.hpp.md#terminalcell)* -- null when that line is no longer held

One line of text by absolute line number, whether it is still on the live grid or has aged into scrollback. Lines that have fallen off the end of the scrollback are gone and report null.

> `len` is the row's own width, which is not always the grid's. A row captured before the terminal was widened keeps the width it had, so every caller has to stop at `len` rather than at m_cols -- reading to the wider grid's edge walks off the end of the row.

---

### hitTestLine

```cpp
hitTestLine(const Vec2f& pos)
```

**Parameters**

- `const Vec2f& pos`

**Returns** — long -- absolute line number under the position

Which line of text a point falls on, taking the scrolled-back view into account.

---

### hitTestColumn

```cpp
hitTestColumn(const Vec2f& pos)
```

**Parameters**

- `const Vec2f& pos`

**Returns** — int -- column under the position

Which column a point falls on. Rounding rather than truncating puts the boundary at the middle of a cell, so dragging across a character either takes all of it or none of it.

---

### normalizedSelection

```cpp
normalizedSelection(long& fromLine, int& fromCol, long& toLine, int& toCol)
```

**Parameters**

- `long& fromLine`
- `int& fromCol`
- `long& toLine`
- `int& toCol`

**Returns** — void

The selection ordered so the start comes before the end, since a drag upward or leftward leaves the anchor after the head.

---

### selectedText

```cpp
selectedText()
```

**Returns** — std::string

The selected text. A selection spanning several lines takes the first line from its start column to the end, the last from its beginning to the end column, and everything in between whole. Trailing blanks go, so the padding a cell grid keeps does not come along with the paste.

---

### clearSelection

```cpp
clearSelection()
```

**Returns** — void

Drops the selection and repaints without the highlight.

---

### selectAll

```cpp
selectAll()
```

**Returns** — void

Selects the scrollback and the live screen together.

---

### selectRange

```cpp
selectRange(long fromLine, int fromCol, long toLine, int toCol)
```

**Parameters**

- `long fromLine`
- `int fromCol`
- `long toLine`
- `int toCol`

**Returns** — void

Selects a range of absolute line numbers. Nothing is clamped to what is currently held: a range reaching past the end of the scrollback simply yields the part that survives.

---

### copySelection

```cpp
copySelection()
```

**Returns** — void

Puts the selection on the system clipboard, leaving it alone when nothing is selected.

---

### pasteClipboard

```cpp
pasteClipboard()
```

**Returns** — void

Sends the clipboard to the shell as though it had been typed. Carriage returns are normalised to newlines, since a pasted CRLF would otherwise submit a line twice. When the running program has asked for bracketed paste the text is fenced, which is what lets an editor take a multi-line paste literally instead of acting on every newline in it.

---

### checkLeftClick

```cpp
checkLeftClick(const Vec2f& mousePos)
```

**Parameters**

- `const Vec2f& mousePos`

**Returns** — bool -- true when the click landed on the terminal

Takes focus, so keystrokes start going to the shell.

---

### checkHover

```cpp
checkHover(const Vec2f& mousePos)
```

**Parameters**

- `const Vec2f& mousePos`

**Returns** — bool -- true when the pointer is over the terminal

Asks for the text cursor while the pointer is inside.

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

**Returns** — bool -- true when the terminal consumed the event

Scrolls back through history. The shell is never told: as far as it is concerned the screen has not moved, which is why scrolling back during a running program does not disturb it. Declines when there is no history, so the event bubbles to the page.

---

### onDeactivate

```cpp
onDeactivate()
```

**Returns** — void

Releases focus when something else is clicked, so typing stops reaching the shell and the cursor stops blinking.

---

### handleTextInput

```cpp
handleTextInput(char32_t unicode)
```

**Parameters**

- `char32_t unicode`

**Returns** — void

Sends a typed character to the shell. Nothing is echoed locally: the shell decides what appears, which is what makes a password prompt hide input without the widget knowing anything about it.

---

### sendKey

```cpp
sendKey(SDL_Keycode key, bool shift, bool ctrl)
```

**Parameters**

- `SDL_Keycode key`
- `bool shift`
- `bool ctrl`

**Returns** — void

Encodes a non-printing key the way a terminal is expected to. Arrows and Home/End switch between the normal CSI forms and the SS3 forms when the program has asked for application cursor keys -- getting that wrong is what makes arrow keys print letters inside a full-screen editor. Ctrl with a letter becomes the matching control character, which is how interrupt and end-of-file reach the shell at all.

---

### handleKeyInput

```cpp
handleKeyInput(SDL_Keycode key, bool shift, bool ctrl)
```

**Parameters**

- `SDL_Keycode key`
- `bool shift`
- `bool ctrl`

**Returns** — void

Routes a key to the shell, and snaps the view back to the bottom first -- typing while scrolled back should show what is being typed. Printable characters arrive through handleTextInput instead, so only the keys that need encoding are handled here.
