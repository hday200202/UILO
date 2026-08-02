# Row.cpp

`include/elements/containers/Row.cpp`

[← index](../../README.md)

## Functions

- [`resolveScrollBounds(...)`](#resolvescrollbounds)
- [`canUseLooseScrollBounds(const RowOptions& options, float contentMax)`](#canuseloosescrollbounds)
- [`normalizeGridStep(float step, float ratio, float minDistance, float maxDistance)`](#normalizegridstep)
- [`Row(...)`](#row)
- [`contentOverflow()`](#contentoverflow)
- [`scrollStep(bool precise)`](#scrollstep)
- [`readScrollLinks()`](#readscrolllinks)
- [`publishScrollLinks()`](#publishscrolllinks)
- [`applyScrollOffset(float target)`](#applyscrolloffset)
- [`update(Rectf& parentBounds, float dt)`](#update)
- [`updateScrollable(float dt, float scale)`](#updatescrollable)
- [`updateFlow(float dt, float scale)`](#updateflow)
- [`layoutResizers(float dt, float scale)`](#layoutresizers)
- [`tickHiddenChildren(float dt)`](#tickhiddenchildren)
- [`render()`](#render)
- [`renderBackground(float scale)`](#renderbackground)
- [`renderSubdivisions(float scale)`](#rendersubdivisions)
- [`renderChildren()`](#renderchildren)
- [`renderChildPass(bool ignoreScrollChildren, bool viewportInset)`](#renderchildpass)
- [`checkZoom(const Vec2f& mousePosition, float magnification)`](#checkzoom)
- [`setZoomX(float z)`](#setzoomx)
- [`checkScroll(const Vec2f& mousePosition, float delta, bool precise, bool momentum)`](#checkscroll)
- [`checkScroll(const Vec2f& mousePosition, Vec2f delta, bool precise, bool momentum)`](#checkscroll)

---

### resolveScrollBounds

```cpp
resolveScrollBounds(...)
```

**Parameters**

- `const RowOptions& options`
- `float contentMax`
- `float& minScroll`
- `float& maxScroll`

**Returns** — void

Works out how far a row may scroll. With both explicit bounds set the options win outright, swapped into order if they arrive reversed; otherwise the range runs from 0 to however much content overflows the viewport.

---

### canUseLooseScrollBounds

```cpp
canUseLooseScrollBounds(const RowOptions& options, float contentMax)
```

**Parameters**

- `const RowOptions& options`
- `float contentMax`

**Returns** — bool

Whether a scroll event should be consumed at all. Explicit bounds always allow it; otherwise there has to be overflow to scroll into, so a row whose content fits lets the event bubble to its parent.

---

### normalizeGridStep

```cpp
normalizeGridStep(float step, float ratio, float minDistance, float maxDistance)
```

**Parameters**

- `float step`
- `float ratio`
- `float minDistance`
- `float maxDistance`

**Returns** — float

Coarsens or refines a grid step by whole ratio steps until it lands between the two thresholds. This is what keeps the subdivision grid at a stable on-screen density while zooming, instead of the lines crowding together or drifting apart.

---

### Row

```cpp
Row(...)
```

**Parameters**

- `Modifier modifier`
- `RowOptions options`
- `contains children`
- `const std::string& name`

**Returns** — [Row](Row.hpp.md#row)

Constructs a row from a modifier, its options and its children, and tags it as a [Row](Row.hpp.md#row) so layout and hit-testing can identify it.

---

### contentOverflow

```cpp
contentOverflow()
```

**Returns** — float

How much content sticks out past the scrolling viewport, never negative. This is the distance the row can travel when no explicit scroll bounds were set.

---

### scrollStep

```cpp
scrollStep(bool precise)
```

**Parameters**

- `bool precise`

**Returns** — float

Pixels to travel per unit of scroll delta. A trackpad reports a precise pixel delta and is scaled against scrollSpeed differently from a wheel's discrete step, since the OS already supplies the momentum tail.

---

### readScrollLinks

```cpp
readScrollLinks()
```

**Returns** — void

Takes the shared scroll offset and zoom from the link groups this row belongs to. Linked rows travel together, so the shared value has to be adopted before layout rather than after it.

---

### publishScrollLinks

```cpp
publishScrollLinks()
```

**Returns** — void

Writes this row's scroll offset and zoom back to its link groups, so every other row sharing the same id picks them up on its next update.

---

### applyScrollOffset

```cpp
applyScrollOffset(float target)
```

**Parameters**

- `float target`

**Returns** — void

Moves the row to an absolute offset, clamped into the range the current content allows, then publishes it to any linked row and marks the row dirty.

---

### update

```cpp
update(Rectf& parentBounds, float dt)
```

**Parameters**

- `Rectf& parentBounds`
- `float dt`

**Returns** — void

Lays the row's children out along the horizontal axis and ticks each one with the slot it was given. Resolves the row's own bounds, adopts any linked scroll offset and zoom, then hands the children to whichever of the two layout paths applies: updateScrollable when the row scrolls, updateFlow when it does not. Resizers and hidden children are dealt with afterwards in both cases, since neither takes part in the flow.

---

### updateScrollable

```cpp
updateScrollable(float dt, float scale)
```

**Parameters**

- `float dt`
- `float scale`

**Returns** — void

Lays out a scrolling row. Pinned children -- those with [Modifier](../Modifier.hpp.md#modifier)::ignoreScroll -- are placed first, grouped left, centre and right, and the width they take is reserved: whatever is left between the left and right strips becomes the scrolling viewport, so a pinned header stays put while the rest of the row travels. The scrolling children are then placed from the viewport's left edge displaced by the scroll offset, their total measured as the content width, and the offset finally clamped into the range that content allows and published to any linked row.

---

### updateFlow

```cpp
updateFlow(float dt, float scale)
```

**Parameters**

- `float dt`
- `float scale`

**Returns** — void

Lays out a non-scrolling row. Children are sorted into left, centre and right buckets by their alignment, then the width left over after the fixed-size ones is shared among the percent-sized ones -- every percent child gets the same slot, so two 50% siblings beside a 100px one split what remains rather than the whole row. Each group is finally placed from its own anchor: left from the left edge, centre about the middle, right against the right edge.

---

### layoutResizers

```cpp
layoutResizers(float dt, float scale)
```

**Parameters**

- `float dt`
- `float scale`

**Returns** — void

Places the row's [Resizer](../interactible/Resizer.hpp.md#resizer) children, which sit at the boundary between their two nearest visible neighbours rather than in the layout flow, so they take no space from it. The boundary is the midpoint of the gap between those neighbours, padding included, and falls back to the container edge when a resizer has a neighbour on only one side. Vertically the resizer spans whatever its neighbours have in common. Its hit width comes from the [Modifier](../Modifier.hpp.md#modifier) like any other element, centred on the boundary, and it is given the neighbour its direction points at as the element it drags.

---

### tickHiddenChildren

```cpp
tickHiddenChildren(float dt)
```

**Parameters**

- `float dt`

**Returns** — void

Ticks the children layout skipped, but only on a forced tree update. A hidden child takes no space and is never placed, so without this it would keep whatever bounds it had when it was last visible; giving it a zero-size slot at the row's own origin clears them instead.

---

### render

```cpp
render()
```

**Returns** — void

Draws the row: background, then the subdivision grid, then its children. The grid goes between the two so it sits behind every child. A [Material](../../utils/Material.hpp.md#material) on the [Modifier](../Modifier.hpp.md#modifier) makes the whole subtree one glass group, so the children composite into the blur the background established rather than each blurring separately.

---

### renderBackground

```cpp
renderBackground(float scale)
```

**Parameters**

- `float scale`

**Returns** — void

Draws the row's background. A [Material](../../utils/Material.hpp.md#material) takes precedence and owns the background outright -- it draws its own rounded rect, tint and effect, so the flat fill is skipped rather than drawn underneath it. Otherwise an active gradient wins over the flat colour, and either is drawn with whatever outline the options ask for. Nothing is drawn at all when the fill is transparent and there is no outline.

---

### renderSubdivisions

```cpp
renderSubdivisions(float scale)
```

**Parameters**

- `float scale`

**Returns** — void

Draws the subdivision grid behind the row's children, positioned against the scroll offset so it travels with the content. Only a scrollable row draws one. The step is normalised to a stable on- screen density, so zooming coarsens or refines the grid rather than crowding the lines together; minor lines subdivide each major step and are drawn fainter, and an optional stripe fills alternating bands of major steps. Every set is batched into one draw call, and only the part crossing the viewport is emitted.

---

### renderChildren

```cpp
renderChildren()
```

**Returns** — void

Draws the children in two passes, scrolling content first and pinned content over the top, so a pinned child is never buried by content sliding underneath it.

---

### renderChildPass

```cpp
renderChildPass(bool ignoreScrollChildren, bool viewportInset)
```

**Parameters**

- `bool ignoreScrollChildren`
- `bool viewportInset`

**Returns** — void

Draws one pass of children, either the scrolling ones or the pinned ones. Each is clipped to the row so content cannot spill past a rounded corner, and scrolling content is clipped again to the scroll viewport so it cannot bleed into the strip a pinned child owns and be sliced by that child's background.

---

### checkZoom

```cpp
checkZoom(const Vec2f& mousePosition, float magnification)
```

**Parameters**

- `const Vec2f& mousePosition`
- `float magnification`

**Returns** — bool -- true when the row consumed the gesture

Applies a pinch or scroll-zoom to the row's horizontal zoom, offering it to the children first so a nested zoomable element wins. The scroll offset is then rewritten so the content under the cursor stays under the cursor: the content coordinate at the pointer is read at the old zoom, then converted back into an offset at the new one.

---

### setZoomX

```cpp
setZoomX(float z)
```

**Parameters**

- `float z`

**Returns** — void

Sets the horizontal zoom directly, clamped to the configured range. Unlike checkZoom this does not preserve the point under the cursor, since there is no gesture to anchor to.

---

### checkScroll

```cpp
checkScroll(const Vec2f& mousePosition, float delta, bool precise, bool momentum)
```

**Parameters**

- `const Vec2f& mousePosition`
- `float delta`
- `bool precise`
- `bool momentum`

**Returns** — bool -- true when the row consumed the event

Single-axis scroll, offered to the children first. A row scrolls horizontally, so it declines a zero delta and lets plain vertical wheel events bubble to a parent that can use them. `precise` marks a trackpad's pixel delta, which is scaled differently from a wheel's discrete step.

---

### checkScroll

```cpp
checkScroll(const Vec2f& mousePosition, Vec2f delta, bool precise, bool momentum)
```

**Parameters**

- `const Vec2f& mousePosition`
- `Vec2f delta`
- `bool precise`
- `bool momentum`

**Returns** — bool -- true when the row consumed the event

Two-axis scroll. The children are offered the full delta first, but their answer is deliberately not short-circuited: a row owns delta.x and applies it whatever a parent [Column](Column.hpp.md#column) does with delta.y, so a nested pair scrolls on both axes from one gesture. Falls back to the [Modifier](../Modifier.hpp.md#modifier)'s onScroll handler when nothing consumed the event.
