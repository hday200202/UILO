# ColorPicker.hpp

`include/elements/widgets/ColorPicker.hpp`

[← index](../../README.md)

## Types

- [ColorPickerOptions](#colorpickeroptions)
- [ColorPicker](#colorpicker)

---

### ColorPickerOptions

[Theme](../../utils/Theme.hpp.md#theme) roles and change notification for an inline HSV/alpha picker. The caller owns popup placement and apply/cancel semantics.

---

### ColorPicker

Saturation/value field, vertical hue and opacity rails, preview swatch and editable hex text. Programmatic changes are silent; user edits notify once. Achromatic colors retain the last chosen hue.
