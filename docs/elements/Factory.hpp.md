# Factory.hpp

`include/elements/Factory.hpp`

[← index](../README.md)

## Functions

- [`heading(std::string_view role, const std::string& content, const std::string& name)`](#heading)
- [`labelledButton(std::string_view role, std::string_view labelRole, const std::string& caption, F&& onClick, const std::string& name)`](#labelledbutton)

---

### heading

```cpp
heading(std::string_view role, const std::string& content, const std::string& name)
```

**Parameters**

- `std::string_view role`
- `const std::string& content`
- `const std::string& name`

**Returns** — [Text](decoration/Text.hpp.md#text)*

Shared body of h1/h2/h3 and the other text roles. Both halves of the role are looked up: the [Modifier](Modifier.hpp.md#modifier) for the line box, the [TextOptions](decoration/Text.hpp.md#textoptions) for the glyphs.

---

### labelledButton

```cpp
labelledButton(std::string_view role, std::string_view labelRole, const std::string& caption, F&& onClick, const std::string& name)
```

**Parameters**

- `std::string_view role`
- `std::string_view labelRole`
- `const std::string& caption`
- `F&& onClick`
- `const std::string& name`

**Returns** — [Button](interactible/Button.hpp.md#button)*

Shared body of primaryButton/ghostButton. The label is its own [Text](decoration/Text.hpp.md#text), so it follows the label role rather than repeating a size and an alignment at the call site.

> The [Modifier](Modifier.hpp.md#modifier) takes the same role as the Options, so a theme can give "primary" a width without every call site restating it.
