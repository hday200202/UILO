# Color.hpp

`include/utils/Color.hpp`

[← index](../README.md)

## Types

- [Color](#color)

---

### Color

An 8-bit RGBA colour. Opaque by default, so a colour written without an alpha behaves the way one is usually meant to, and a zero alpha is what the library reads as "nothing to draw" in several places rather than a separate flag.

> Colours are normally paired with a palette role at the call site: the role wins when it resolves, and the literal is the fallback.
