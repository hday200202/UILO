# Palette.cpp

`include/Palette.cpp`

[← index](README.md)

## Functions

- [`set(const std::string& role, Color color)`](#set)
- [`setAlias(const std::string& role, const std::string& target)`](#setalias)
- [`has(std::string_view role)`](#has)
- [`get(std::string_view role)`](#get)
- [`resolve(std::string_view role, Color literal)`](#resolve)
- [`resolveImpl(std::string_view role, int depth)`](#resolveimpl)
- [`setGradient(const std::string& role, const Gradient& gradient)`](#setgradient)
- [`getGradient(std::string_view role)`](#getgradient)
- [`hasGradient(std::string_view role)`](#hasgradient)
- [`clear()`](#clear)

---

### set

```cpp
set(const std::string& role, Color color)
```

**Parameters**

- `const std::string& role`
- `Color color`

**Returns** — void

Assigns a direct color to a role, overwriting any existing entry including an alias.

---

### setAlias

```cpp
setAlias(const std::string& role, const std::string& target)
```

**Parameters**

- `const std::string& role`
- `const std::string& target`

**Returns** — void

Makes `role` resolve to whatever `target` resolves to, overwriting any existing entry. Cycles are tolerated at resolution time, returning the fallback after a depth cap.

---

### has

```cpp
has(std::string_view role)
```

**Parameters**

- `std::string_view role`

**Returns** — bool

True when the role exists in the map as a color or alias. Does not follow aliases to verify they ultimately resolve.

---

### get

```cpp
get(std::string_view role)
```

**Parameters**

- `std::string_view role`

**Returns** — [Color](utils/Color.hpp.md#color)

Returns the color for a role, walking alias chains. Returns the fallback color when unresolved.

---

### resolve

```cpp
resolve(std::string_view role, Color literal)
```

**Parameters**

- `std::string_view role`
- `Color literal`

**Returns** — [Color](utils/Color.hpp.md#color)

Render-path convenience. Returns `literal` when the role is empty or "none", otherwise the palette resolution, which itself falls back to the palette's fallback color if the alias chain dies.

---

### resolveImpl

```cpp
resolveImpl(std::string_view role, int depth)
```

**Parameters**

- `std::string_view role`
- `int depth`

**Returns** — [Color](utils/Color.hpp.md#color)

Recursively resolves a role, following alias chains until it reaches a direct color. Returns the fallback color when the role is missing or the alias depth cap is reached, which breaks cycles.

---

### setGradient

```cpp
setGradient(const std::string& role, const Gradient& gradient)
```

**Parameters**

- `const std::string& role`
- `const Gradient& gradient`

**Returns** — void

Stores a whole gradient under a role so a theme can define it once and every element using that gradient role follows a palette swap.

---

### getGradient

```cpp
getGradient(std::string_view role)
```

**Parameters**

- `std::string_view role`

**Returns** — const [Gradient](utils/Gradient.hpp.md#gradient)*

Returns the gradient for a role, or nullptr when the role isn't present.

---

### hasGradient

```cpp
hasGradient(std::string_view role)
```

**Parameters**

- `std::string_view role`

**Returns** — bool

True when a gradient is stored under the role.

---

### clear

```cpp
clear()
```

**Returns** — void

Removes all color and gradient entries.
