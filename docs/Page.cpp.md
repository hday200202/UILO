# Page.cpp

`include/Page.cpp`

[← index](README.md)

## Functions

- [`Page(Container* rootContainer, const std::string& name)`](#page)
- [`update(Rectf& screenBounds, float dt)`](#update)
- [`render()`](#render)
- [`setUILO(UILO& uiloRef)`](#setuilo)

---

### Page

```cpp
Page(Container* rootContainer, const std::string& name)
```

**Parameters**

- `Container* rootContainer`
- `const std::string& name`

**Returns** — [Page](Page.hpp.md#page)

Constructs a page from a root container and a name.

---

### update

```cpp
update(Rectf& screenBounds, float dt)
```

**Parameters**

- `Rectf& screenBounds`
- `float dt`

**Returns** — void

Lays out the page by ticking its root container against the given screen bounds and delta time.

---

### render

```cpp
render()
```

**Returns** — void

Draws the page by rendering its root container.

---

### setUILO

```cpp
setUILO(UILO& uiloRef)
```

**Parameters**

- `UILO& uiloRef`

**Returns** — void

Binds the page and its root container to a [UILO](UILO.hpp.md#uilo) instance.
