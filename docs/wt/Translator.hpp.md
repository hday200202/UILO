# Translator.hpp

`include/wt/Translator.hpp`

[← index](../README.md)

## Types

- [Translator](#translator)

---

### Translator

Turns a [UILO](../UILO.hpp.md#uilo) element tree into Wt widgets and keeps them in step. build() walks the tree once per session and creates a widget per element; sync() then runs each frame and only re- applies properties -- styles, text, values -- to widgets that already exist. Structural changes after build() are therefore invisible to the web, which is why widgets that change shape at runtime keep their parts alive and toggle visibility instead of rebuilding.
