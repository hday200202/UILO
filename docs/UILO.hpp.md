# UILO.hpp

`include/UILO.hpp`

[← index](README.md)

## Types

- [WebConfig](#webconfig)
- [WebApp](#webapp)
- [UILO](#uilo)

## Functions

- [`setRenderer(Renderer& renderer)`](#setrenderer)
- [`getPopupBackdrops()`](#getpopupbackdrops)
- [`getElement(const std::string& name)`](#getelement)

---

### WebConfig

Server-side settings for [UILO](#uilo)::runWeb -- the things that have no desktop equivalent because they describe the page and the HTTP server, not the UI. The UI itself (pages, palette, callbacks) is set on the [UILO](#uilo) instance exactly as on desktop.

---

### WebApp

One browser session's [UILO](#uilo), plus whatever else has to stay alive for as long as that session does. Implement it on the type that owns your [UILO](#uilo) and hand a factory to the per-session runWeb: the server makes one per connection and destroys it when the connection ends, so every browser gets its own pages, palette and application state. The single-instance runWeb needs none of this -- there, every connection shares the one [UILO](#uilo) it was called on.

---

### UILO

The top-level controller. It owns the pages and the element pool, drives layout and input dispatch once per frame, and manages overlays, the floating layer, scaling, cursor requests and the shared scroll and zoom links.

> Ownership runs through here rather than through the tree. An element binds itself with setUILO() on the first walk that reaches it, which puts it in the pool, and erase() only marks it -- the sweep happens between frames, so a handler is free to remove elements while the tree is still being walked.

> The floating layer is the one part of the tree that is ticked, hit-tested and drawn above the page. A popup has to live there rather than as a child, or the container it sits in would clip it.

---

### setRenderer

```cpp
setRenderer(Renderer& renderer)
```

**Parameters**

- `Renderer& renderer`

**Returns** — void

Attaches the renderer this [UILO](#uilo) draws through, and hands its SDL window to the mouse bindings so they can resolve positions against it.

---

### getPopupBackdrops

```cpp
getPopupBackdrops()
```

**Returns** — std::vector&lt;[Element](elements/Element.hpp.md#element)*&gt; -- in insertion order

The elements currently in the floating layer. Native rendering walks the internal list directly; this exists for the web bridge, which cannot see it and reads this on each sync to mirror popups -- a picker adds its backdrop here when it opens -- into an on-top overlay.

---

### getElement

```cpp
getElement(const std::string& name)
```

**Parameters**

- `const std::string& name`

**Returns** — T* -- null when the name is unknown or the type does not match

Looks up a named element and casts it to the requested type. Only elements given a name at construction are registered, and the cast is checked, so asking for the wrong type yields null rather than undefined behaviour.
