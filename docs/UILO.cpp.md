# UILO.cpp

`include/UILO.cpp`

[← index](README.md)

## Functions

- [`UILO(Renderer& renderer, Page* page)`](#uilo)
- [`addPage(Page* page)`](#addpage)
- [`setPage(const std::string& pageName)`](#setpage)
- [`setActivePage(Page* page)`](#setactivepage)
- [`setCurrInteractible(Interactible* i)`](#setcurrinteractible)
- [`setScale(float scale)`](#setscale)
- [`registerOverlay(Element* e, std::function&lt;void()&gt; onDismiss)`](#registeroverlay)
- [`unregisterOverlay(Element* e)`](#unregisteroverlay)
- [`addFloating(FreeElement f)`](#addfloating)
- [`removeFloating(Element* e)`](#removefloating)
- [`requestCursor(CursorType type, int priority)`](#requestcursor)
- [`getScrollLinkOffset(const std::string& linkId, bool horizontal)`](#getscrolllinkoffset)
- [`setScrollLinkOffset(const std::string& linkId, float offset, bool horizontal)`](#setscrolllinkoffset)
- [`getZoomLinkValue(const std::string& linkId, bool horizontal)`](#getzoomlinkvalue)
- [`setZoomLinkValue(const std::string& linkId, float zoom, bool horizontal)`](#setzoomlinkvalue)
- [`setOnLiveResize(std::function&lt;void()&gt; cb)`](#setonliveresize)
- [`update()`](#update)
- [`render()`](#render)
- [`queryMousePixelPosition()`](#querymousepixelposition)
- [`dispatchScroll(const Vec2f& pos, Vec2f delta, bool precise, bool momentum)`](#dispatchscroll)
- [`dispatchZoom(const Vec2f& pos, float magnification)`](#dispatchzoom)
- [`wantsScrollMomentum(const Vec2f& pos)`](#wantsscrollmomentum)
- [`handleEvent(const SDL_Event& event)`](#handleevent)
- [`handleEvent(const SDL_Event& event)`](#handleevent)

---

### UILO

```cpp
UILO(Renderer& renderer, Page* page)
```

**Parameters**

- `Renderer& renderer`
- `Page* page`

**Returns** — [UILO](UILO.hpp.md#uilo)

Set's [UILO](UILO.hpp.md#uilo)'s renderer reference, adds the [Page](Page.hpp.md#page) to [UILO](UILO.hpp.md#uilo), sets active page

---

### addPage

```cpp
addPage(Page* page)
```

**Parameters**

- `Page* page`

**Returns** — void

Registers a page with [UILO](UILO.hpp.md#uilo), taking ownership and binding it to this instance. Keyed by the page's name.

---

### setPage

```cpp
setPage(const std::string& pageName)
```

**Parameters**

- `const std::string& pageName`

**Returns** — void

Makes the named page active, clearing overlays, resizers, floating elements, and the current interactible.

---

### setActivePage

```cpp
setActivePage(Page* page)
```

**Parameters**

- `Page* page`

**Returns** — void

Sets the active page without taking ownership. No-op when the page is already active so per-frame calls don't reset the overlay, floating, and interactible state.

---

### setCurrInteractible

```cpp
setCurrInteractible(Interactible* i)
```

**Parameters**

- `Interactible* i`

**Returns** — void

Sets the focused interactible, deactivating the previous one, and starts or stops SDL text input depending on whether the new interactible wants keyboard text.

---

### setScale

```cpp
setScale(float scale)
```

**Parameters**

- `float scale`

**Returns** — void

Sets the global UI scale factor. Ignores non-positive values.

---

### registerOverlay

```cpp
registerOverlay(Element* e, std::function<void()> onDismiss)
```

**Parameters**

- `Element* e`
- `std::function<void()> onDismiss`

**Returns** — void

Registers an element as a modal overlay with an optional dismiss callback. Ignores duplicates.

---

### unregisterOverlay

```cpp
unregisterOverlay(Element* e)
```

**Parameters**

- `Element* e`

**Returns** — void

Removes an element from the overlay list.

---

### addFloating

```cpp
addFloating(FreeElement f)
```

**Parameters**

- `FreeElement f`

**Returns** — [Element](elements/Element.hpp.md#element)*

Adds a free-floating element at a fixed or percent position, optionally draggable, and returns it. Registers the element and any child elements with [UILO](UILO.hpp.md#uilo).

---

### removeFloating

```cpp
removeFloating(Element* e)
```

**Parameters**

- `Element* e`

**Returns** — void

Removes a floating element by pointer.

---

### requestCursor

```cpp
requestCursor(CursorType type, int priority)
```

**Parameters**

- `CursorType type`
- `int priority`

**Returns** — void

Requests a cursor for this frame. The highest-priority request wins; ties keep the most recent.

---

### getScrollLinkOffset

```cpp
getScrollLinkOffset(const std::string& linkId, bool horizontal)
```

**Parameters**

- `const std::string& linkId`
- `bool horizontal`

**Returns** — float

Returns the shared scroll offset for a link id on the given axis, or 0 when unset.

---

### setScrollLinkOffset

```cpp
setScrollLinkOffset(const std::string& linkId, float offset, bool horizontal)
```

**Parameters**

- `const std::string& linkId`
- `float offset`
- `bool horizontal`

**Returns** — void

Stores a shared scroll offset for a link id on the given axis so linked containers scroll together.

---

### getZoomLinkValue

```cpp
getZoomLinkValue(const std::string& linkId, bool horizontal)
```

**Parameters**

- `const std::string& linkId`
- `bool horizontal`

**Returns** — float

Returns the shared zoom value for a link id on the given axis, or 1 when unset.

---

### setZoomLinkValue

```cpp
setZoomLinkValue(const std::string& linkId, float zoom, bool horizontal)
```

**Parameters**

- `const std::string& linkId`
- `float zoom`
- `bool horizontal`

**Returns** — void

Stores a shared zoom value for a link id on the given axis so linked containers zoom together.

---

### setOnLiveResize

```cpp
setOnLiveResize(std::function<void()> cb)
```

**Parameters**

- `std::function<void()> cb`

**Returns** — void

Sets a callback invoked during native live window resize so the host can redraw at each intermediate size.

---

### update

```cpp
update()
```

**Returns** — void

Advances the UI by one frame. On the first frame -- and only when [UILO](UILO.hpp.md#uilo) owns its window, never when embedded in a host -- it installs the macOS native scroll and zoom monitors and the live- resize configuration. Each frame it converts the SDL mouse position from logical points to backing pixels, updates layout for the active page and every floating element, culls elements marked for deletion, then dispatches hover, left-click, and right-click input. Floating elements are opaque to input: the topmost one under the cursor consumes hover and clicks so the page beneath is shielded, and draggable ones follow the cursor while held.

---

### render

```cpp
render()
```

**Returns** — void

Draws the active page, then floating elements, overlays, and resizers in back-to-front order.

---

### queryMousePixelPosition

```cpp
queryMousePixelPosition()
```

**Returns** — [Vec2f](utils/Math.hpp.md#vec2f)

The cursor in render pixels. SDL reports it in window points, so the ratio between the window's point and pixel size converts it; on a retina display those differ by the backing scale.

---

### dispatchScroll

```cpp
dispatchScroll(const Vec2f& pos, Vec2f delta, bool precise, bool momentum)
```

**Parameters**

- `const Vec2f& pos`
- `Vec2f delta`
- `bool precise`
- `bool momentum`

**Returns** — void

Routes a scroll delta at a position to the topmost overlay under it, or the active page's root otherwise. For a real gesture the cached cursor is refreshed first, so callbacks reading getMousePosition() see the position that triggered the scroll. Momentum ticks leave it alone: they replay at the gesture's origin, which is no longer where the pointer is.

---

### dispatchZoom

```cpp
dispatchZoom(const Vec2f& pos, float magnification)
```

**Parameters**

- `const Vec2f& pos`
- `float magnification`

**Returns** — void

Routes a zoom magnification at a position to the topmost overlay under it, or the active page's root otherwise.

---

### wantsScrollMomentum

```cpp
wantsScrollMomentum(const Vec2f& pos)
```

**Parameters**

- `const Vec2f& pos`

**Returns** — bool

Walks the visible element tree from the topmost overlay (or the page root) down to the deepest element containing the position, and asks that element whether a flick over it should coast. The macOS monitor uses this both to decide whether to start a coast at the end of a gesture and to stop one that has drifted onto something that does not want it.

> Nothing here routes events to SDL: the monitor consumes every precise scroll it sees, and a mouse wheel never reaches it in the first place.

---

### handleEvent

```cpp
handleEvent(const SDL_Event& event)
```

**Parameters**

- `const SDL_Event& event`

**Returns** — void

Processes one SDL event. Mouse-wheel events map to zoom when Ctrl/Cmd is held, horizontal scroll when Shift is held, or normal scroll otherwise. [Text](elements/decoration/Text.hpp.md#text) input and key input route to the focused interactible, with a filter that drops the stale key- repeat events Wayland can deliver after a key is released. UTF-8 text is decoded one codepoint at a time so batched or IME input is not dropped.

---

### handleEvent

```cpp
handleEvent(const SDL_Event& event)
```

**Parameters**

- `const SDL_Event& event`

**Returns** — void

Turns one SDL event into [UILO](UILO.hpp.md#uilo)'s own dispatch. Scroll and zoom gestures are separated here -- a wheel with a modifier held is a zoom, not a scroll -- and keyboard events are routed to the active interactible, except for the zoom shortcuts and the navigation keys, which drive scrolling when nothing has focus. A key repeat arriving after every key has physically come up is dropped, which is what stops a held key from continuing to fire once the window has lost and regained focus.
