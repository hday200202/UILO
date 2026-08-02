# Cursor.hpp

`include/utils/Cursor.hpp`

[← index](../README.md)

## Types

- [CursorType](#cursortype)

---

### CursorType

The mouse cursor shapes [UILO](../UILO.hpp.md#uilo) can ask the platform for. Requests are made per frame and resolved by priority, so the shape follows whatever is under the pointer without anyone having to reset it on the way out.

> Lives in its own header so [Modifier](../elements/Modifier.hpp.md#modifier) can carry one without pulling in the renderer.
