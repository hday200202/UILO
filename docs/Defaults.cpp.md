# Defaults.cpp

`include/Defaults.cpp`

[← index](README.md)

## Functions

- [`installDefaultTheme(Theme& theme)`](#installdefaulttheme)

---

### installDefaultTheme

```cpp
installDefaultTheme(Theme& theme)
```

**Parameters**

- `Theme& theme`

**Returns** — void

Fills a theme with defaultTheme(). [Theme](utils/Theme.hpp.md#theme).hpp declares this and this file defines it, which is what keeps the dependency pointing one way: every *Options header includes [Theme](utils/Theme.hpp.md#theme).hpp, so [Theme](utils/Theme.hpp.md#theme).hpp including Defaults.hpp -- and through it every element type -- would close a cycle. Only this translation unit needs to know both halves.
