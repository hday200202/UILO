# Modifier.hpp

`include/elements/Modifier.hpp`

[← index](../README.md)

## Types

- [cb_traits](#cb-traits)
- [Modifier](#modifier)

## Functions

- [`makeClickCb(F&& f)`](#makeclickcb)
- [`makeScrollCb(F&& f)`](#makescrollcb)
- [`setOnLeftClick(F&& f)`](#setonleftclick)
- [`setOnRightClick(F&& f)`](#setonrightclick)
- [`setOnHoverEnter(F&& f)`](#setonhoverenter)
- [`setOnHoverExit(F&& f)`](#setonhoverexit)
- [`setOnHover(F&& f)`](#setonhover)
- [`setOnScroll(F&& f)`](#setonscroll)
- [`setOnUpdateStart(F&& f)`](#setonupdatestart)
- [`setOnUpdateEnd(F&& f)`](#setonupdateend)

---

### cb_traits

First-argument introspection for a callable, so a user-supplied handler can be written as `[](){}`, `[](Element*){}` or `[](Button*){}` without ceremony. Reports the arity and the first parameter type; specialised for member operator() in both const and non-const forms, and for plain function pointers.

---

### Modifier

The layout and event half of an element, kept separate from the per-widget Options that describe how it is drawn. Carries size, alignment, outer padding, visibility, free position, material, and the callback set: clicks, edge-triggered hover, scroll, and the two per-frame lifecycle hooks. Every setter returns *this so a modifier reads as one declaration at the call site.

> There is deliberately no continuous "is hovered" callback: handlers react to the enter and exit transitions and keep any persistent visual change on the element themselves.

---

### makeClickCb

```cpp
makeClickCb(F&& f)
```

**Parameters**

- `F&& f`

**Returns** — FuncPtr

Wraps a click- or hover-style callable into the canonical signature. A no-argument callable is called with the self- pointer discarded; a one-argument callable has the pointer cast to the type it asked for, which is what lets a handler take [Button](interactible/Button.hpp.md#button)* directly. Anything else fails a static_assert with the accepted shapes spelled out.

---

### makeScrollCb

```cpp
makeScrollCb(F&& f)
```

**Parameters**

- `F&& f`

**Returns** — ScrollFuncPtr

Wraps a scroll-style callable into the canonical signature. A one-argument callable is taken as `(float delta)` and called with the self-pointer discarded; a two-argument one gets the pointer cast to the type it asked for.

---

### setOnLeftClick

```cpp
setOnLeftClick(F&& f)
```

**Parameters**

- `F&& f`

**Returns** — [Modifier](#modifier)&

Left-click handler. Accepts a no-argument lambda, one taking a generic [Element](Element.hpp.md#element)*, or one taking the concrete element type, and wraps it down to the canonical signature.

---

### setOnRightClick

```cpp
setOnRightClick(F&& f)
```

**Parameters**

- `F&& f`

**Returns** — [Modifier](#modifier)&

Right-click handler, accepting the same callable shapes as setOnLeftClick.

---

### setOnHoverEnter

```cpp
setOnHoverEnter(F&& f)
```

**Parameters**

- `F&& f`

**Returns** — [Modifier](#modifier)&

Fires once when the cursor first enters the element's bounds.

---

### setOnHoverExit

```cpp
setOnHoverExit(F&& f)
```

**Parameters**

- `F&& f`

**Returns** — [Modifier](#modifier)&

Fires once when the cursor leaves the element's bounds.

---

### setOnHover

```cpp
setOnHover(F&& f)
```

**Parameters**

- `F&& f`

**Returns** — [Modifier](#modifier)&

Back-compatible alias for setOnHoverEnter, which is how the older single hover callback always behaved.

---

### setOnScroll

```cpp
setOnScroll(F&& f)
```

**Parameters**

- `F&& f`

**Returns** — [Modifier](#modifier)&

Scroll handler. Accepts either `(float delta)` or `(ElementPtr, float delta)`.

---

### setOnUpdateStart

```cpp
setOnUpdateStart(F&& f)
```

**Parameters**

- `F&& f`

**Returns** — [Modifier](#modifier)&

Fires at the top of every update tick, before layout and state are recomputed. Receives the element, so a handler can read the current bounds or drive options from per-frame state.

---

### setOnUpdateEnd

```cpp
setOnUpdateEnd(F&& f)
```

**Parameters**

- `F&& f`

**Returns** — [Modifier](#modifier)&

Fires at the end of every update tick, once layout has run.
