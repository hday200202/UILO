# Container.cpp

`include/elements/containers/Container.cpp`

[← index](../../README.md)

## Functions

- [`Container(Modifier modifier, contains children, const std::string& name)`](#container)
- [`isDirty()`](#isdirty)
- [`checkLeftClick(const Vec2f& mousePosition)`](#checkleftclick)
- [`checkRightClick(const Vec2f& mousePosition)`](#checkrightclick)
- [`checkHover(const Vec2f& mousePosition)`](#checkhover)
- [`checkScroll(const Vec2f& mousePosition, float delta, bool precise, bool momentum)`](#checkscroll)
- [`checkScroll(const Vec2f& mousePosition, Vec2f delta, bool precise, bool momentum)`](#checkscroll)
- [`checkZoom(const Vec2f& mousePosition, float magnification)`](#checkzoom)
- [`tickFloating(float dt)`](#tickfloating)
- [`renderFloating(float rounding)`](#renderfloating)
- [`beginFloatingDrag(const Vec2f& mousePosition)`](#beginfloatingdrag)
- [`addElement(Element* element)`](#addelement)
- [`pruneChildren()`](#prunechildren)
- [`setUILO(UILO& uiloRef)`](#setuilo)
- [`collectResizers(std::vector&lt;Element*&gt;& out)`](#collectresizers)

---

### Container

```cpp
Container(Modifier modifier, contains children, const std::string& name)
```

**Parameters**

- `Modifier modifier`
- `contains children`
- `const std::string& name`

**Returns** — [Container](Container.hpp.md#container)

Constructs a container from a modifier and an initializer list of children, reserving room for them up front.

---

### isDirty

```cpp
isDirty()
```

**Returns** — bool

True when this container or anything below it needs redrawing, so a change deep in the tree still reaches the top.

---

### checkLeftClick

```cpp
checkLeftClick(const Vec2f& mousePosition)
```

**Parameters**

- `const Vec2f& mousePosition`

**Returns** — bool -- true when the click was claimed here or below

Offers the click to every child under the cursor first, and only fires the container's own handler when none of them took it. That ordering is what lets a decorative child -- a [Text](../decoration/Text.hpp.md#text) label, an indent [Spacer](../decoration/Spacer.hpp.md#spacer) -- sit inside a clickable row without swallowing the press. Resizers are skipped; they are hit-tested separately because they sit outside the layout flow.

---

### checkRightClick

```cpp
checkRightClick(const Vec2f& mousePosition)
```

**Parameters**

- `const Vec2f& mousePosition`

**Returns** — bool -- true when the click was claimed here or below

As checkLeftClick, for the right button.

---

### checkHover

```cpp
checkHover(const Vec2f& mousePosition)
```

**Parameters**

- `const Vec2f& mousePosition`

**Returns** — bool -- true when anything in this subtree took the hover

Recurses into every child unconditionally, not just the ones under the cursor, so a child that has just been left still fires its own onHoverExit instead of staying stuck hovered. The container claims the hover itself only when no child did and the cursor is inside it, and requests the hand cursor when it has a click handler of its own.

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

**Returns** — bool -- true when the event was consumed

Offers the scroll to the children under the cursor, stopping at the first that takes it, and otherwise falls back to the [Modifier](../Modifier.hpp.md#modifier)'s onScroll handler. A plain [Container](Container.hpp.md#container) does not scroll itself -- [Row](Row.hpp.md#row) and [Column](Column.hpp.md#column) override this.

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

**Returns** — bool -- true when the event was consumed

The two-axis form of the above, passing the whole delta down.

---

### checkZoom

```cpp
checkZoom(const Vec2f& mousePosition, float magnification)
```

**Parameters**

- `const Vec2f& mousePosition`
- `float magnification`

**Returns** — bool -- true when the gesture was consumed

Offers the zoom to every child under the cursor without stopping at the first taker, so the deepest or last-drawn one wins. A [Canvas](Canvas.hpp.md#canvas) drawn over its siblings needs that to claim the gesture.

---

### tickFloating

```cpp
tickFloating(float dt)
```

**Parameters**

- `float dt`

**Returns** — void

Places and updates the container's floating children. Their slot is the content area rather than a share of the flow, so a free position of zero puts one in the container's top-left corner, inside its inner padding. A drag in progress is applied first, so the child lands at its new position in the same frame it moved rather than one frame behind the pointer.

---

### renderFloating

```cpp
renderFloating(float rounding)
```

**Parameters**

- `float rounding`

**Returns** — void

Draws the floating children above everything else the container holds, clipped to the container so a floating panel cannot spill out of the pane it belongs to. `rounding` comes from the container's own Options, which [Container](Container.hpp.md#container) cannot see.

---

### beginFloatingDrag

```cpp
beginFloatingDrag(const Vec2f& mousePosition)
```

**Parameters**

- `const Vec2f& mousePosition`

**Returns** — bool -- true when a drag was started

Starts dragging the topmost draggable floating child under the pointer. The grab offset is kept so the element does not jump to centre itself on the cursor.

---

### addElement

```cpp
addElement(Element* element)
```

**Parameters**

- `Element* element`

**Returns** — void

Appends a child. When the container is already bound to a [UILO](../../UILO.hpp.md#uilo) the new child is bound too, so an element added at runtime lands in the element pool and can resolve palette roles like any other.

---

### pruneChildren

```cpp
pruneChildren()
```

**Returns** — void

Drops children that have been erased. Called at the top of layout rather than at erase() time, because a click handler is free to erase an element while the parent is still walking its child list.

---

### setUILO

```cpp
setUILO(UILO& uiloRef)
```

**Parameters**

- `UILO& uiloRef`

**Returns** — void

Binds this container and, recursively, every child to a [UILO](../../UILO.hpp.md#uilo). This is what puts each element in the pool and gives it access to the palette and scale.

---

### collectResizers

```cpp
collectResizers(std::vector<Element*>& out)
```

**Parameters**

- `std::vector<Element*>& out`

**Returns** — void

Gathers every [Resizer](../interactible/Resizer.hpp.md#resizer) in the subtree. [UILO](../../UILO.hpp.md#uilo) hit-tests them ahead of the tree walk, so it needs them in one flat list.
