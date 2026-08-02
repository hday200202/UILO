# UILO.hpp

`include/UILO.hpp`

[← index](README.md)

## Types

- [UILO](#uilo)

## Functions

- [`setRenderer(Renderer& renderer)`](#setrenderer)
- [`getFloatingElements()`](#getfloatingelements)
- [`getPalette()`](#getpalette)
- [`ownPalette()`](#ownpalette)
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

Attaches the renderer this [UILO](#uilo) draws through, and hands its SDL window to the mouse bindings so they can resolve positions against it.

---

### getFloatingElements

```cpp
getFloatingElements()
```

**Returns** — std::vector&lt;[Element](elements/Element.hpp.md#element)*&gt; -- in insertion order

The elements currently in the floating layer. Native rendering walks the internal list directly; this exists for the web bridge, which cannot see it and reads this on each sync to mirror popups -- a picker adds its backdrop here when it opens -- into an on-top overlay.

---

### getPalette

```cpp
getPalette()
```

**Returns** — const [Palette](Palette.hpp.md#palette)&

The palette colours resolve against: this [UILO](#uilo)'s own when it has one, otherwise the Theme's. Read live rather than cached, so replacing the theme's palette restyles every [UILO](#uilo) that has not overridden it and theme switching needs no rebuild.

---

### ownPalette

```cpp
ownPalette()
```

**Returns** — [Palette](Palette.hpp.md#palette)& -- this [UILO](#uilo)'s own, to edit in place

Takes ownership of a palette: the theme's is copied in on the first call and this [UILO](#uilo) stops following Theme::setPalette() from then on. Deliberately not an overload of getPalette(), because that ownership transfer is too surprising to happen merely because the caller happened to hold a non-const [UILO](#uilo) -- reading through getPalette() always keeps following the theme.

---

### getElement

```cpp
getElement(const std::string& name)
```

**Parameters**

- `const std::string& name`

**Returns** — T* -- null when the name is unknown or the type does not match

Looks up a named element and casts it to the requested type. Only elements given a name at construction are registered, and the cast is checked, so asking for the wrong type yields null rather than undefined behaviour.
