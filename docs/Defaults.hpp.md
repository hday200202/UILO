# Defaults.hpp

`include/Defaults.hpp`

[← index](README.md)

## Functions

- [`darkPalette()`](#darkpalette)
- [`lightPalette()`](#lightpalette)
- [`defaultTheme()`](#defaulttheme)

---

### darkPalette

```cpp
darkPalette()
```

**Returns** — [Palette](Palette.hpp.md#palette)

The colour roles every built-in widget reads, in their dark values. A palette that covers these nine names themes the whole library without a single role being spelled at a call site; everything else here is an alias pointing at one of them.

---

### lightPalette

```cpp
lightPalette()
```

**Returns** — [Palette](Palette.hpp.md#palette)

darkPalette()'s roles in their light values, so switching the two restyles a running application without rebuilding anything: colours resolve through the palette every frame.

---

### defaultTheme

```cpp
defaultTheme()
```

**Returns** — [Theme](utils/Theme.hpp.md#theme)

Everything above assembled into the theme a [UILO](UILO.hpp.md#uilo) starts on. Build from this rather than from a bare [Theme](utils/Theme.hpp.md#theme){} when overriding a few things, since a [Theme](utils/Theme.hpp.md#theme) constructed from scratch defines no prototypes at all and leaves every type on its built-in baseline.
