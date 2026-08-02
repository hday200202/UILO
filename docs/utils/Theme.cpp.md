# Theme.cpp

`include/utils/Theme.cpp`

[← index](../README.md)

## Functions

- [`current()`](#current)
- [`set(const Theme& theme)`](#set)
- [`reset()`](#reset)
- [`resolveRounding(const std::optional&lt;float&gt;& own, float fallback)`](#resolverounding)
- [`resolveOuterPadding(const std::optional&lt;float&gt;& own, float fallback)`](#resolveouterpadding)
- [`resolveCharSize(const std::optional&lt;unsigned int&gt;& own, unsigned int fallback)`](#resolvecharsize)
- [`resolveIconStrokeWidth(const std::optional&lt;float&gt;& own, float fallback)`](#resolveiconstrokewidth)
- [`resolveFont(const std::string& own)`](#resolvefont)
- [`hasPalette()`](#haspalette)
- [`palette()`](#palette)

---

### current

```cpp
current()
```

**Returns** — Theme&

The process-wide theme. A function-local static rather than a namespace-scope one so it is constructed on first use, which makes it safe to touch from a static initializer.

---

### set

```cpp
set(const Theme& theme)
```

**Parameters**

- `const Theme& theme`

**Returns** — void

Replaces the active theme outright, dropping anything the previous one said.

---

### reset

```cpp
reset()
```

**Returns** — void

Clears every themed property, so every type goes back to its own default.

---

### resolveRounding

```cpp
resolveRounding(const std::optional<float>& own, float fallback)
```

**Parameters**

- `const std::optional<float>& own`
- `float fallback`

**Returns** — float

The element's own radius, the theme's, or the type's default, in that order.

---

### resolveOuterPadding

```cpp
resolveOuterPadding(const std::optional<float>& own, float fallback)
```

**Parameters**

- `const std::optional<float>& own`
- `float fallback`

**Returns** — float

As resolveRounding(), for an element's inset.

---

### resolveCharSize

```cpp
resolveCharSize(const std::optional<unsigned int>& own, unsigned int fallback)
```

**Parameters**

- `const std::optional<unsigned int>& own`
- `unsigned int fallback`

**Returns** — unsigned int

As resolveRounding(), for text size.

---

### resolveIconStrokeWidth

```cpp
resolveIconStrokeWidth(const std::optional<float>& own, float fallback)
```

**Parameters**

- `const std::optional<float>& own`
- `float fallback`

**Returns** — float

As resolveRounding(), for icon stroke weight.

---

### resolveFont

```cpp
resolveFont(const std::string& own)
```

**Parameters**

- `const std::string& own`

**Returns** — const std::string&

The element's own font path when it has one, otherwise the theme's. An empty result means the embedded default face, which is what an unset font has always meant.

---

### hasPalette

```cpp
hasPalette()
```

**Returns** — bool

Whether the theme carries a palette. [UILO](../UILO.hpp.md#uilo) consults this when it has not been given one of its own.

---

### palette

```cpp
palette()
```

**Returns** — const [Palette](../Palette.hpp.md#palette)&

The themed palette, or an empty one when none is set. Never dangles: the empty fallback is a static, so this is always safe to resolve roles against (they simply fall through to the literal colors).
