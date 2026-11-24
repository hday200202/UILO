# UILO.hpp

`include/UILO.hpp`

[← index](README.md)

## Types

- [UILO](#uilo)

## Functions

- [`setRenderer(Renderer& renderer)`](#setrenderer)
- [`getElement(const std::string& name)`](#getelement)

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

Attaches the renderer this [UILO](#uilo) draws through, and hands its window to the key and mouse bindings so they can read input from it.

---

### getElement

```cpp
getElement(const std::string& name)
```

**Parameters**

- `const std::string& name`

**Returns** — T* -- null when the name is unknown or the type does not match

Looks up a named element and casts it to the requested type. Only elements given a name at construction are registered, and the cast is checked, so asking for the wrong type yields null rather than undefined behaviour.
