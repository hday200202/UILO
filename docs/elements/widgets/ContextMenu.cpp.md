# ContextMenu.cpp

`include/elements/widgets/ContextMenu.cpp`

[← index](../../README.md)

## Functions

- [`ContextMenu(Modifier modifier, ContextMenuOptions options, const std::string& name)`](#contextmenu)
- [`applySurface()`](#applysurface)
- [`makeSlot()`](#makeslot)
- [`growTo(size_t count)`](#growto)
- [`rowWidthFor(const ContextMenuItem& item, bool showIconColumn)`](#rowwidthfor)
- [`measure()`](#measure)
- [`applySlot(Slot& slot, const ContextMenuItem& item, bool showIconColumn)`](#applyslot)
- [`hideSlot(Slot& slot)`](#hideslot)
- [`setItems(const std::vector&lt;ContextMenuItem&gt;& items)`](#setitems)
- [`paintRow(size_t i, bool hovered)`](#paintrow)
- [`highlight(int index)`](#highlight)
- [`moveHighlight(int delta)`](#movehighlight)
- [`activateHighlighted()`](#activatehighlighted)
- [`submenuFor(size_t index)`](#submenufor)
- [`openHighlightedSubmenu()`](#openhighlightedsubmenu)
- [`closeSubmenu()`](#closesubmenu)
- [`deepestOpen()`](#deepestopen)
- [`openSubmenu()`](#opensubmenu)
- [`show(float preferredLeft, float flipRightEdge, float top, bool flipVertically)`](#show)
- [`openAt(const Vec2f& cursor)`](#openat)
- [`close()`](#close)
- [`closeChain()`](#closechain)
- [`update(Rectf& parentBounds, float dt)`](#update)

---

### ContextMenu

```cpp
ContextMenu(Modifier modifier, ContextMenuOptions options, const std::string& name)
```

**Parameters**

- `Modifier modifier`
- `ContextMenuOptions options`
- `const std::string& name`

**Returns** — [ContextMenu](ContextMenu.hpp.md#contextmenu)

Builds an empty menu. It stays empty until setItems(), and holds no rows at all until then, so a [UILO](../../UILO.hpp.md#uilo) that is never right-clicked pays for nothing but the object.

---

### applySurface

```cpp
applySurface()
```

**Returns** — void

Pushes the panel fill, rounding, border and vertical inset down onto the [Column](../containers/Column.hpp.md#column) the menu is. Inner padding insets all four sides, which is what keeps the highlight off the rounded corners.

---

### makeSlot

```cpp
makeSlot()
```

**Returns** — [ContextMenu](ContextMenu.hpp.md#contextmenu)::Slot

Creates one position's worth of elements and appends them to the column: the item row, then the separator rule that position could be instead. Both start hidden. The row's parts are created once and only ever relabelled, so the structure the web bridge translated stays valid.

---

### growTo

```cpp
growTo(size_t count)
```

**Parameters**

- `size_t count`

**Returns** — void

Makes sure the pool has at least `count` positions. The pool only ever grows: a menu that once needed eight rows keeps them, hidden, so the next one costs no allocation and the translated structure is never invalidated.

---

### rowWidthFor

```cpp
rowWidthFor(const ContextMenuItem& item, bool showIconColumn)
```

**Parameters**

- `const ContextMenuItem& item`
- `bool showIconColumn`

**Returns** — float -- the row's natural width in render pixels

What one row needs: the two insets, the icon column when the menu has one, the measured label, and room for the submenu arrow. Falls back to a per-character estimate when no renderer is bound, so a headless probe still produces sane numbers.

---

### measure

```cpp
measure()
```

**Returns** — [Vec2f](../../utils/Math.hpp.md#vec2f) -- the size the current items need, in render pixels

Width from the widest row, clamped between the configured minimum and maximum; height from the rows themselves plus the vertical inset above and below. openAt() places exactly this.

---

### applySlot

```cpp
applySlot(Slot& slot, const ContextMenuItem& item, bool showIconColumn)
```

**Parameters**

- `Slot& slot`
- `const ContextMenuItem& item`
- `bool showIconColumn`

**Returns** — void

Points one pooled position at one item: shows the rule for a separator, or the row for anything else, and hides the parts that item does not use. A disabled item keeps its place and its label and only loses its colour, which is what makes it read as "not right now" rather than as missing.

---

### hideSlot

```cpp
hideSlot(Slot& slot)
```

**Parameters**

- `Slot& slot`

**Returns** — void

Takes a pooled position out of use. Both shapes are hidden, so the slot occupies no height and is skipped by layout.

---

### setItems

```cpp
setItems(const std::vector<ContextMenuItem>& items)
```

**Parameters**

- `const std::vector<ContextMenuItem>& items`

**Returns** — void

Fills the menu. Grows the pool if these items need more positions than it has, points each position at its item, hides the rest, and wires every row's click to either its action or its submenu.

---

### paintRow

```cpp
paintRow(size_t i, bool hovered)
```

**Parameters**

- `size_t i`
- `bool hovered`

**Returns** — void

Repaints every row from the current highlight and hover state. Takes an index only so callers read naturally; the whole menu is cheap enough to repaint in one pass, and doing so is what keeps a stale highlight from being left behind.

---

### highlight

```cpp
highlight(int index)
```

**Parameters**

- `int index`

**Returns** — void

Moves the keyboard highlight, closing an open submenu when it leaves the row that opened it.

---

### moveHighlight

```cpp
moveHighlight(int delta)
```

**Parameters**

- `int delta -- +1 for the next item`
- `-1 for the previous`

**Returns** — bool -- true when the key was consumed

Steps the highlight over separators and disabled items, wrapping at both ends. Applies to the deepest open submenu, so the arrow keys always drive the menu the user is looking at.

---

### activateHighlighted

```cpp
activateHighlighted()
```

**Returns** — bool -- true when the key was consumed

Picks the highlighted item, which opens it if it is a submenu and otherwise runs its action and closes the whole chain.

---

### submenuFor

```cpp
submenuFor(size_t index)
```

**Parameters**

- `size_t index`

**Returns** — [ContextMenu](ContextMenu.hpp.md#contextmenu)* -- the child menu belonging to that row

The submenu for a row, created the first time that row needs one and kept afterwards. Inherits the parent's options, so a menu is styled once however deep it nests.

---

### openHighlightedSubmenu

```cpp
openHighlightedSubmenu()
```

**Returns** — bool -- true when a submenu opened

Opens the highlighted row's submenu beside that row, flipped to the left when it would run off the right edge. The two overlap slightly, so the pointer never crosses a gap on the way over.

---

### closeSubmenu

```cpp
closeSubmenu()
```

**Returns** — bool -- true when a submenu was open and is now closed

Closes the deepest open submenu, which is what Escape and the left arrow do: they back out one level rather than dismissing the whole menu.

---

### deepestOpen

```cpp
deepestOpen()
```

**Returns** — [ContextMenu](ContextMenu.hpp.md#contextmenu)* -- the innermost menu currently showing

Which menu the keyboard is driving. Walks down the open submenu chain rather than up, since the innermost one has focus.

---

### openSubmenu

```cpp
openSubmenu()
```

**Returns** — [ContextMenu](ContextMenu.hpp.md#contextmenu)* -- the open child menu, or null

One level down, for a caller that has to visit every menu in an open chain rather than only the innermost.

---

### show

```cpp
show(float preferredLeft, float flipRightEdge, float top, bool flipVertically)
```

**Parameters**

- `float preferredLeft`
- `float flipRightEdge`
- `float top`
- `bool flipVertically`

**Returns** — void

Places the menu and shows it. It takes `preferredLeft` when it fits, and otherwise puts its right edge at `flipRightEdge` -- two anchors rather than one because the two callers flip differently: a menu at the cursor flips about the cursor, while a submenu has to land on the far side of its parent. Registering it as an overlay is what gets it hit-tested above the page and dismissed by a click outside.

> Whatever the anchors say, the menu is finally clamped into the window, so on a window narrower than the menu it sits against the edge and lets the overflow show rather than moving somewhere unexpected.

---

### openAt

```cpp
openAt(const Vec2f& cursor)
```

**Parameters**

- `const Vec2f& cursor`

**Returns** — void

Shows the menu with its top-left at the cursor, flipping about the cursor on either axis when it would not fit below and to the right of it.

---

### close

```cpp
close()
```

**Returns** — void

Hides the menu and any submenu under it, and takes it out of the overlay layer. The rows are kept, so reopening allocates nothing.

---

### closeChain

```cpp
closeChain()
```

**Returns** — void

Closes this menu and every menu that opened it, so picking an item anywhere in a nested chain puts the whole thing away.

---

### update

```cpp
update(Rectf& parentBounds, float dt)
```

**Parameters**

- `Rectf& parentBounds`
- `float dt`

**Returns** — void

Lays the rows out as an ordinary [Column](../containers/Column.hpp.md#column) would, then repaints the highlight. The paint runs after layout because it reads each row's hovered state, which the dispatch sets from the bounds this pass resolves.
