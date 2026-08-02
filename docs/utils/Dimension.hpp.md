# Dimension.hpp

`include/utils/Dimension.hpp`

[← index](../README.md)

## Types

- [Dimension](#dimension)

---

### Dimension

A size given either as absolute pixels or as a percentage of whatever the parent offers. The `percent` flag says which, and resolve() turns the pair into a concrete number against a parent size. Percent children share the space left over after their fixed-size siblings have taken theirs, which is why the two are kept distinct rather than resolved up front.

> Write them with the _px and _pct literals -- 10_px, 50_pct -- rather than constructing the struct directly.
