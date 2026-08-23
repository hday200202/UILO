# Themed.hpp

`include/utils/Themed.hpp`

[← index](../README.md)

## Types

- [Themed](#themed)

## Functions

- [`inheritOptional(T& own, const T& prototype)`](#inheritoptional)
- [`inheritRole(std::string& own, const std::string& prototype)`](#inheritrole)

---

### Themed

A field that remembers whether anyone set it. An element is built before it belongs to a [UILO](../UILO.hpp.md#uilo), so the theme cannot be consulted at construction -- it is applied later, when the element binds. That only works if a field can tell "the caller chose this" from "nobody said", which is the whole of what this carries: a value, and one bit.

```cpp
Themed<bool> m_bold{false};   // false is the baseline
m_bold.set(true);             // explicit; a theme cannot
                              // overwrite it
m_bold.inherit(proto.m_bold); // fills in only if unset
```


> Fields that already have a way to say "unset" do not need this. std::optional carries the bit itself, and a colour role uses "" for no role, so both are left as they are and merged with the same rule.

> get() is what the getters return, so nothing outside an Options class ever sees this type: converting a field costs one line in its setter and one in its getter, and every caller keeps compiling.

---

### inheritOptional

```cpp
inheritOptional(T& own, const T& prototype)
```

**Parameters**

- `T& own`
- `const T& prototype`

**Returns** — void

The same rule for a std::optional field, which already carries its own "was it set" bit.

---

### inheritRole

```cpp
inheritRole(std::string& own, const std::string& prototype)
```

**Parameters**

- `std::string& own`
- `const std::string& prototype`

**Returns** — void

The same rule for a colour-role field, where "" already means "no role was named".
