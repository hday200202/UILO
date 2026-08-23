# Palette.hpp

`include/Palette.hpp`

[← index](README.md)

## Types

- [Palette](#palette)

---

### Palette

A flat name->[Color](utils/Color.hpp.md#color) map with alias and named-gradient support, owned by [UILO](UILO.hpp.md#uilo). Elements resolve role strings to colors at draw time via [UILO](UILO.hpp.md#uilo)::getPalette(). An empty role or "none" means "no role" and falls back to the element's literal color; an unresolved non-empty role falls back to the literal, then to the palette's fallback color. Aliases let one role point at another so a small base of colors drives many widget-specific roles, with cycles broken after a depth cap.

> A Palette is a container, not a look. The colours themselves live in Defaults.hpp and reach elements through the [Theme](utils/Theme.hpp.md#theme) that owns them, so there is no way to install one without going through a [Theme](utils/Theme.hpp.md#theme).
