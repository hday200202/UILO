# Element.cpp

`include/elements/Element.cpp`

[← index](../README.md)

## Functions

- [`getBounds()`](#getbounds)
- [`getModifier()`](#getmodifier)
- [`isDirty()`](#isdirty)
- [`erase()`](#erase)
- [`getType()`](#gettype)
- [`getDeltaTime()`](#getdeltatime)
- [`resolveColor(std::string_view role, Color literal)`](#resolvecolor)
- [`resolveGradient(...)`](#resolvegradient)
- [`tick(Rectf& parentBounds, float dt)`](#tick)
- [`resize(const Rectf& parent)`](#resize)
- [`contentArea()`](#contentarea)
- [`claimsPointerEvents()`](#claimspointerevents)
- [`checkLeftClick(const Vec2f& mousePosition)`](#checkleftclick)
- [`checkRightClick(const Vec2f& mousePosition)`](#checkrightclick)
- [`checkHover(const Vec2f& mousePosition)`](#checkhover)
- [`checkScroll(const Vec2f& mousePosition, float delta, bool precise, bool momentum)`](#checkscroll)
- [`setUILO(UILO& uiloRef)`](#setuilo)

---

### getBounds

```cpp
getBounds()
```

**Returns** — [Rectf](../utils/Math.hpp.md#rectf)

The element's resolved position and size in render pixels, as of the last layout pass.

---

### getModifier

```cpp
getModifier()
```

**Returns** — [Modifier](Modifier.hpp.md#modifier)&

The element's live modifier. Mutable, so a handler can change size, alignment, visibility or another callback at runtime.

---

### isDirty

```cpp
isDirty()
```

**Returns** — bool

Whether the element needs redrawing. Containers override this to also report a dirty descendant.

---

### erase

```cpp
erase()
```

**Returns** — void

Marks the element for deletion rather than deleting it. [UILO](../UILO.hpp.md#uilo) sweeps marked elements out of the pool between frames, so this is safe to call from a handler while the tree is being walked.

---

### getType

```cpp
getType()
```

**Returns** — [ElementType](Element.hpp.md#elementtype)

Which kind of element this is, for the few places that branch on type instead of on a virtual call.

---

### getDeltaTime

```cpp
getDeltaTime()
```

**Returns** — float

Seconds since the previous frame, read from the owning [UILO](../UILO.hpp.md#uilo). 0 when the element is not bound to one, so per-frame animation in a handler degrades to standing still rather than jumping.

---

### resolveColor

```cpp
resolveColor(std::string_view role, Color literal)
```

**Parameters**

- `std::string_view role`
- `Color literal`

**Returns** — [Color](../utils/Color.hpp.md#color)

Resolves a color through the owning [UILO](../UILO.hpp.md#uilo)'s [Palette](../Palette.hpp.md#palette). Returns `literal` unchanged when there is no [UILO](../UILO.hpp.md#uilo), or when the role is empty, "none", or names nothing the palette knows -- so a role is always safe to set and a palette is never required.

---

### resolveGradient

```cpp
resolveGradient(...)
```

**Parameters**

- `const Gradient& literal`
- `std::string_view gradientRole`
- `Color out[4]`

**Returns** — bool -- true when the result is worth drawing

Picks the gradient to draw and resolves its stops to colors. A non-empty role naming a palette gradient wins over the literal one, and each stop's own role is then resolved. Fills `out` in TL, TR, BL, BR order, and reports false for an inactive gradient or one that came out fully transparent so the caller can fall back to a flat fill.

---

### tick

```cpp
tick(Rectf& parentBounds, float dt)
```

**Parameters**

- `Rectf& parentBounds`
- `float dt`

**Returns** — void

Non-virtual wrapper that fires onUpdateStart and onUpdateEnd around the virtual update(). Everything that walks the tree -- containers, [Page](../Page.hpp.md#page), a dropdown popup -- calls this rather than update() directly, so the lifecycle hooks always run.

---

### resize

```cpp
resize(const Rectf& parent)
```

**Parameters**

- `const Rectf& parent`

**Returns** — void

Works out the element's own bounds inside the slot its parent gave it. A percent dimension is taken against the slot; a pixel dimension is multiplied by [UILO](../UILO.hpp.md#uilo)'s scale. Outer padding shrinks the element on all four sides and insets the box it is aligned within, so padding never moves a sibling. The element is then placed against that inset box according to its alignment flags, defaulting to the top-left corner.

> A floating element skips all of that after sizing: it is placed at its free position relative to the slot's corner, since it is outside the flow and has neither siblings to be spaced from nor an alignment box to sit in.

---

### contentArea

```cpp
contentArea()
```

**Returns** — [Rectf](../utils/Math.hpp.md#rectf)

Where this element's content belongs: its bounds inset on all four sides by the inner padding its Options reports. The element still draws its own background and is hit-tested against the full bounds, so inner padding moves the content and nothing else.

---

### claimsPointerEvents

```cpp
claimsPointerEvents()
```

**Returns** — bool

Whether this element claims a pointer event that lands on it, stopping the search there. An element with nothing attached is transparent, so a decorative child -- the [Text](decoration/Text.hpp.md#text) label of a clickable row, a [Spacer](decoration/Spacer.hpp.md#spacer) used for indentation, a background [Image](decoration/Image.hpp.md#image) -- does not swallow the click or hover of the container it sits in. Widgets that are interactive by nature override this to true, so a press over a [Button](interactible/Button.hpp.md#button) or [Slider](interactible/Slider.hpp.md#slider) never falls through to whatever is behind it even when no callback was attached.

---

### checkLeftClick

```cpp
checkLeftClick(const Vec2f& mousePosition)
```

**Parameters**

- `const Vec2f& mousePosition`

**Returns** — bool -- true when the element claimed the click

Fires the [Modifier](Modifier.hpp.md#modifier)'s left-click handler when the position is inside the element, and reports whether the click should stop here.

---

### checkRightClick

```cpp
checkRightClick(const Vec2f& mousePosition)
```

**Parameters**

- `const Vec2f& mousePosition`

**Returns** — bool -- true when the element claimed the click

As checkLeftClick, for the right button.

---

### checkHover

```cpp
checkHover(const Vec2f& mousePosition)
```

**Parameters**

- `const Vec2f& mousePosition`

**Returns** — bool -- true when the element claimed the hover

Tracks the hovered state, fires onHoverEnter and onHoverExit on the transitions, and asks for the element's cursor. The enter and exit bookkeeping always runs, but only an element that actually wants pointer events masks its parent's hover, so a decorative child does not blank out the row it sits in.

> The cursor is requested on every frame the pointer is inside, not once on the way in: [UILO](../UILO.hpp.md#uilo) clears the request pool at the top of each frame, so a one-shot request would be undone immediately. A [Modifier](Modifier.hpp.md#modifier) cursor wins over the hand a clickable element asks for, which is what lets an explicit Arrow suppress it.

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

Fires the [Modifier](Modifier.hpp.md#modifier)'s onScroll handler when the position is inside the element. A plain element does not scroll itself, so without that handler the event is declined and bubbles to a parent that can use it.

---

### setUILO

```cpp
setUILO(UILO& uiloRef)
```

**Parameters**

- `UILO& uiloRef`

**Returns** — void

Binds the element to a [UILO](../UILO.hpp.md#uilo), which puts it in the element pool and registers its name for lookup. Re-binding to the same [UILO](../UILO.hpp.md#uilo) is a no-op: switching the active page from A to B and back re- walks A's tree, and emplacing the element a second time would leave two unique_ptrs owning it and double-delete at shutdown.
