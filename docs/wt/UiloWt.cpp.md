# UiloWt.cpp

`include/wt/UiloWt.cpp`

[← index](../README.md)

## Types

- [UiloApplication](#uiloapplication)

## Functions

- [`run(int argc, char** argv, Builder build, Config config)`](#run)

---

### UiloApplication

One browser session. Owns that session's [UILO](../UILO.hpp.md#uilo) instance, the tree the builder produced, and the Wt widgets translated from it. Implements [Session](UiloWt.hpp.md#session), so the builder never sees the Wt side.

---

### run

```cpp
run(int argc, char** argv, Builder build, Config config)
```

**Parameters**

- `int argc`
- `char** argv`
- `Builder build`
- `Config config`

**Returns** — int

Starts the Wt application server with the caller's builder. Each browser session gets its own [UILO](../UILO.hpp.md#uilo) and [Translator](Translator.hpp.md#translator), so sessions share the Theme but never each other's element trees or palettes.
