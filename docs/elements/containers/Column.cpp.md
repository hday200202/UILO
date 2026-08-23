# Column.cpp

`include/elements/containers/Column.cpp`

[← index](../../README.md)

## Functions

- [`resolveScrollBounds(...)`](#resolvescrollbounds)
- [`canUseLooseScrollBounds(const ColumnOptions& options, float contentMax)`](#canuseloosescrollbounds)
- [`normalizeGridStep(float step, float ratio, float minDistance, float maxDistance)`](#normalizegridstep)
- [`Column(...)`](#column)
- [`contentOverflow()`](#contentoverflow)
- [`scrollStep(bool precise)`](#scrollstep)
- [`readScrollLinks()`](#readscrolllinks)
- [`publishScrollLinks()`](#publishscrolllinks)
- [`applyScrollOffset(float target)`](#applyscrolloffset)
- [`setScrollOffset(float offset)`](#setscrolloffset)
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
- [`setZoomY(float z)`](#setzoomy)
- [`checkScroll(const Vec2f& mousePosition, float delta, bool precise, bool momentum)`](#checkscroll)
- [`checkScroll(const Vec2f& mousePosition, Vec2f delta, bool precise, bool momentum)`](#checkscroll)

---

### resolveScrollBounds

```cpp
resolveScrollBounds(...)
```

**Parameters**

- `const ColumnOptions& options`
- `float contentMax`
- `float& minScroll`
- `float& maxScroll`

**Returns** — void

Works out how far a column may scroll. With both explicit bounds set the options win outright, swapped into order if they arrive reversed; otherwise the range runs from 0 to however much content overflows the viewport.

---

### canUseLooseScrollBounds

```cpp
canUseLooseScrollBounds(const ColumnOptions& options, float contentMax)
```

**Parameters**

- `const ColumnOptions& options`
- `float contentMax`

**Returns** — bool

Whether a scroll event should be consumed at all. Explicit bounds always allow it; otherwise there has to be overflow to scroll into, so a column whose content fits lets the event bubble to its parent.

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

### Column

```cpp
Column(...)
```

**Parameters**

- `Modifier modifier`
- `ColumnOptions options`
- `contains children`
- `const std::string& name`

**Returns** — [Column](Column.hpp.md#column)

Constructs a column from a modifier, its options and its children, and tags it as a [Column](Column.hpp.md#column) so layout and hit-testing can identify it.

---

### contentOverflow

```cpp
contentOverflow()
```

**Returns** — float

How much content sticks out past the scrolling viewport, never negative. This is the distance the column can travel when no explicit scroll bounds were set.

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

Takes the shared scroll offset and zoom from the link groups this column belongs to. Linked columns travel together, so the shared value has to be adopted before layout rather than after it.

---

### publishScrollLinks

```cpp
publishScrollLinks()
```

**Returns** — void

Writes this column's scroll offset and zoom back to its link groups, so every other column sharing the same id picks them up on its next update.

---

### applyScrollOffset

```cpp
applyScrollOffset(float target)
```

**Parameters**

- `float target`

**Returns** — void

Moves the column to an absolute offset, clamped into the range the current content allows, then publishes it to any linked column and marks the column dirty.

---

### setScrollOffset

```cpp
setScrollOffset(float offset)
```

**Parameters**

- `float offset`

**Returns** — void

Scrolls to an absolute offset, clamped to the range the current content allows. Used to drive a column from somewhere other than a scroll gesture. Measures the overflow against the column's own height rather than the scrolling viewport, so it can be called before the first layout pass has run.

---

### update

```cpp
update(Rectf& parentBounds, float dt)
```

**Parameters**

- `Rectf& parentBounds`
- `float dt`

**Returns** — void

Lays the column's children out along the vertical axis and ticks each one with the slot it was given. Resolves the column's own bounds, adopts any linked scroll offset and zoom, then hands the children to whichever of the two layout paths applies: updateScrollable when the column scrolls, updateFlow when it does not. Resizers and hidden children are dealt with afterwards in both cases, since neither takes part in the flow.

---

### updateScrollable

```cpp
updateScrollable(float dt, float scale)
```

**Parameters**

- `float dt`
- `float scale`

**Returns** — void

Lays out a scrolling column. Pinned children -- those with [Modifier](../Modifier.hpp.md#modifier)::ignoreScroll -- are placed first, grouped top, centre and bottom, and the height they take is reserved: whatever is left between the top and bottom strips becomes the scrolling viewport, so a pinned header stays put while the rest of the column travels. The scrolling children are then placed from the viewport's top edge displaced by the scroll offset, their total measured as the content height, and the offset finally clamped into the range that content allows and published to any linked column.

> A percent height here is a percentage OF the viewport rather than a share of it, since a column whose content overflows on purpose has no leftover space to divide: a lone 50% child is half the viewport tall, and three of them overflow it by half.

---

### updateFlow

```cpp
updateFlow(float dt, float scale)
```

**Parameters**

- `float dt`
- `float scale`

**Returns** — void

Lays out a non-scrolling column. Children are sorted into top, centre and bottom buckets by their alignment, then sized: a pixel height is itself, a percent height is that percentage of the content area, and whatever those two leave over is divided among the _flex children in proportion to their values. Each group is finally placed from its own anchor: top from the top edge, centre about the middle, bottom against the bottom edge.

> Percent and pixel heights are therefore independent of what the siblings ask for, and can overflow the column between them. Only _flex adapts, which is what makes it the sensible default.

---

### layoutResizers

```cpp
layoutResizers(float dt, float scale)
```

**Parameters**

- `float dt`
- `float scale`

**Returns** — void

Places the column's [Resizer](../interactible/Resizer.hpp.md#resizer) children, which sit at the boundary between their two nearest visible neighbours rather than in the layout flow, so they take no space from it. The boundary is the midpoint of the gap between those neighbours, padding included, and falls back to the container edge when a resizer has a neighbour on only one side. Horizontally the resizer is sized from its [Modifier](../Modifier.hpp.md#modifier) width, clamped to whatever span its neighbours have in common and aligned within it, so a handle can be a short bar rather than the full width. Its hit height comes from the [Modifier](../Modifier.hpp.md#modifier) and is centred on the boundary, and it is given the neighbour its direction points at as the element it drags.

---

### tickHiddenChildren

```cpp
tickHiddenChildren(float dt)
```

**Parameters**

- `float dt`

**Returns** — void

Ticks the children layout skipped, but only on a forced tree update. A hidden child takes no space and is never placed, so without this it would keep whatever bounds it had when it was last visible; giving it a zero-size slot at the column's own origin clears them instead.

---

### render

```cpp
render()
```

**Returns** — void

Draws the column: background, then the subdivision grid, then its children. The grid goes between the two so it sits behind every child. A [Material](../../utils/Material.hpp.md#material) on the [Modifier](../Modifier.hpp.md#modifier) makes the whole subtree one glass group, so the children composite into the blur the background established rather than each blurring separately.

---

### renderBackground

```cpp
renderBackground(float scale)
```

**Parameters**

- `float scale`

**Returns** — void

Draws the column's background. A [Material](../../utils/Material.hpp.md#material) takes precedence and owns the background outright -- it draws its own rounded rect, tint and effect, so the flat fill is skipped rather than drawn underneath it. Otherwise an active gradient wins over the flat colour, and either is drawn with whatever outline the options ask for. Nothing is drawn at all when the fill is transparent and there is no outline.

---

### renderSubdivisions

```cpp
renderSubdivisions(float scale)
```

**Parameters**

- `float scale`

**Returns** — void

Draws the subdivision grid behind the column's children, positioned against the scroll offset so it travels with the content. Only a scrollable column draws one. The step is normalised to a stable on-screen density, so zooming coarsens or refines the grid rather than crowding the lines together; minor lines subdivide each major step and are drawn fainter, and an optional stripe fills alternating bands of major steps. Every set is batched into one draw call, and only the part crossing the viewport is emitted.

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

Draws one pass of children, either the scrolling ones or the pinned ones. Each is clipped to the column so content cannot spill past a rounded corner, and scrolling content is clipped again to the scroll viewport so it cannot bleed into the strip a pinned child owns and be sliced by that child's background.

---

### checkZoom

```cpp
checkZoom(const Vec2f& mousePosition, float magnification)
```

**Parameters**

- `const Vec2f& mousePosition`
- `float magnification`

**Returns** — bool -- true when the column consumed the gesture

Applies a pinch or scroll-zoom to the column's vertical zoom, offering it to the children first so a nested zoomable element wins. The scroll offset is then rewritten so the content under the cursor stays under the cursor: the content coordinate at the pointer is read at the old zoom, then converted back into an offset at the new one.

---

### setZoomY

```cpp
setZoomY(float z)
```

**Parameters**

- `float z`

**Returns** — void

Sets the vertical zoom directly, clamped to the configured range. Unlike checkZoom this does not preserve the point under the cursor, since there is no gesture to anchor to.

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

**Returns** — bool -- true when the column consumed the event

Single-axis scroll, offered to the children first. `precise` marks a trackpad's pixel delta, which is scaled against scrollSpeed differently from a wheel's discrete step because the OS already supplies the momentum tail.

> A floating child under the pointer takes the event and the column does not scroll, so the view does not slide out from under a panel laid over it.

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

**Returns** — bool -- true when the column consumed the event

Two-axis scroll. The children are offered the full delta first, but their answer is deliberately not short-circuited: a column owns delta.y and applies it whatever a child [Row](Row.hpp.md#row) does with delta.x, so a nested pair scrolls on both axes from one gesture. Falls back to the [Modifier](../Modifier.hpp.md#modifier)'s onScroll handler when nothing consumed the event.
