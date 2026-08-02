# Material.hpp

`include/utils/Material.hpp`

[← index](../README.md)

## Types

- [Material](#material)

---

### Material

describes a "look" effect applied to an element's background. Materials are carried on a [Modifier](../elements/Modifier.hpp.md#modifier) (`Modifier::setMaterial`). Containers ([Row](../elements/containers/Row.hpp.md#row)/[Column](../elements/containers/Column.hpp.md#column)) and interactible widgets ([Button](../elements/interactible/Button.hpp.md#button), [Dropdown](../elements/interactible/Dropdown.hpp.md#dropdown)) honour the material; everything else ignores it. API --- Each kind is exposed as a *static factory* that returns a Material by value, and Material itself has chainable setters that return `Material&`. This lets you tweak any preset inline at the call site, the same way `Modifier` and `*Options` work elsewhere in [UILO](../UILO.hpp.md#uilo):   Pick a preset and override two fields: row( [Modifier](../elements/Modifier.hpp.md#modifier)() .setMaterial( Material::Blur() .setOpacity(0.4f) .setBlurRadius(12.f)), ...);   Start from scratch (kind = None, no effect): auto m = Material() .setKind(Material::Kind::Glass) .setTint({200, 220, 255, 40}) .setOpacity(0.6f); Available kinds --------------- - None         no effect - Glass        Apple- style frosted glass: backdrop blur + tint + lens - Frosted denser, more matte variant of Glass - Holographic  iridescent shifting tint (animated) - Liquid       sinusoidal UV ripples (animated) - Shimmer      Glass + diagonal highlight sweep (animated) - Aurora       soft drifting magenta/teal colour clouds (animated) - Tinted       glass that uses the element's own colour as tint - Ripple       Tinted + concentric ripples emanating from the cursor - Hover        Tinted + radial highlight under the cursor - Blur         semi-transparent blur of the backdrop, tinted with the element's colour. Per-element blur radius.
