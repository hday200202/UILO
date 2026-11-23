# ColorPicker

[Source](../../../include/elements/widgets/ColorPicker.hpp) · [Implementation](../../../include/elements/widgets/ColorPicker.cpp)

An inline, theme-aware color picker with a saturation/value field, hue rail,
optional opacity rail, checkerboard preview, and editable hex value. Add it to a
container or popup using the usual `Modifier` and `ColorPickerOptions` API.

```cpp
auto* picker = uilo::colorpicker(
    uilo::Modifier().setWidth({260.f, false}).setHeight({240.f, false}),
    uilo::ColorPickerOptions()
        .setAlphaEnabled(true)
        .setOnColorChanged([](uilo::Color color) {
            // Preview or store the edited color.
        }));
picker->setColor(uilo::Color(129, 166, 132));
```

`setColor` silently synchronizes the widget. User drags and `setHex` notify the
change callback when the color changes. Complete six/eight-digit hex values
preview while typing; Enter also accepts three/four-digit shorthand. Invalid
input leaves the color unchanged. Disabling alpha forces opaque colors.

The background, text field, text, and outline use configurable palette roles.
The caller controls popup placement and Apply/Cancel behavior; the widget does
not open a window or save colors itself. Drags stay on the field or rail where
they started and clamp at its edges without hiding or warping the cursor.
