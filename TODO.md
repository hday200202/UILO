# UILO Port TODO

Tracking what's left to port from the old monolithic `UILO` file to the new `include/` structure.
Excludes embedded font data, FileBrowser, and backend-specific concerns (fonts, images, cursors
are the developer's responsibility in the new renderer-agnostic architecture).

---

## Completely Missing Types

### Enums
- [ ] `MouseButton` — used by click dispatch
- [ ] `Key` — used by `onKeyPress`, `TextBox`
- [ ] `ButtonStyle` — `Default`, `Pill`, `Rect`
- [ ] `SliderOrientation` — `Vertical`, `Horizontal`
- [ ] `TBStyle` — bitflag enum: `Default`, `Pill`, `Wrap`, `CenterText`, `Password`

### Structs
- [ ] `FloatRect` — `contains()`, `findIntersection()`. Used by `Bounds`, clipping, `Grid`, etc.
- [ ] `CircleData` — used by `Button` (pill style) and `TextBox` (pill style)

### Classes
- [ ] `Dropdown` — full interactive dropdown with options column, main button, `s_openDropdown` static

---

## Empty Stub Files (declared but no implementation)

### Containers
- [ ] `Grid`
- [ ] `ScrollableColumn`
- [ ] `ScrollableRow`
- [ ] `FreeColumn`

### Interactibles
- [ ] `Button`
- [ ] `Slider`
- [ ] `Knob`
- [ ] `TextBox`

---

## Partially Ported (shell exists, missing content)

- [ ] `Timer` — no members or methods yet
- [ ] `Elements.hpp` — only includes Column/Row/Spacer/Text; missing Grid, ScrollableColumn, ScrollableRow, FreeColumn, FreeRow, Button, Slider, Knob, TextBox
- [ ] `Factory.hpp` — only has `column`, `row`, `spacer`, `text`, `page`; missing `grid`, `scrollableColumn`, `scrollableRow`, `freeColumn`, `button`, `slider`, `knob`, `textbox`, `dropdown`

---

## Ported but with Major API Gaps

### Renderer (callback-based redesign needed)
- [ ] Text measurement — UILO needs text dimensions for layout (TextBox cursor, wrapping). Needs a dev-provided callback (e.g. `onMeasureText`)
- [ ] Clipping — needed by scrollable containers. Needs callbacks (e.g. `onPushClip`/`onPopClip`)
- [ ] `drawCircleOutlined`, `drawTriangleStrip`

### UILO
- [ ] Individual input handlers (`onMousePress`, `onMouseRelease`, `onMouseMove`, `onScroll`, `onTextInput`, `onKeyPress`, `onResize`, `onClose`)
- [ ] Active drag tracking (`m_activeDragSlider`, `m_activeDragKnob`)
- [ ] Per-type registries and getters (`getRow()`, `getButton()`, `getSlider()`, etc.)
- [ ] Dirty/bounds caching
- [ ] `isRunning`, `windowShouldUpdate`, `forceUpdate`, `setInputBlocked`

### Container
- [ ] `addElement()`, `addElements()`
- [ ] `getElements()`, `getElementIndex()`
- [ ] `insertElementAt()`, `removeElement()`, `swapElements()`
- [ ] `clear()`

### Element
- [ ] `m_customRender`, `m_doRender`
- [ ] Per-element `setPosition`/`getPosition`/`getSize`
- [ ] `checkClick` with `MouseButton` arg
- [ ] `updateChildren()`, `applyModifiers()`

### Page
- [ ] Support for multiple containers
- [ ] `dispatchClick`, `dispatchScroll`, `dispatchHover`
- [ ] `clear()`

### Input
- [ ] Keyboard input (`Key` enum, `onKeyPress`, `onTextInput`)
- [ ] Horizontal scroll delta
- [ ] Mouse dragging state
- [ ] Individual mouse button release events
