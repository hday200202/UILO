# Alignment.hpp

`include/utils/Alignment.hpp`

[← index](../README.md)

## Types

- [Align](#align)

## Functions

- [`hasAlign(Align value, Align flag)`](#hasalign)

---

### Align

Where an element sits inside the slot its parent gives it. The horizontal and vertical flags are independent bits, so they combine with `|` -- Align::Left | Align::CenterY places an element against the left edge, centred vertically. A container also reads the flags to bucket its children into start, centre and end groups before laying them out.

---

### hasAlign

```cpp
hasAlign(Align value, Align flag)
```

**Parameters**

- `Align value`
- `Align flag`

**Returns** — bool

Whether a flag is set, which is how layout tests one axis of a combined alignment without disturbing the other.
