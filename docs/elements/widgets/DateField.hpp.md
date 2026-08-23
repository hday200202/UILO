# DateField.hpp

`include/elements/widgets/DateField.hpp`

[← index](../../README.md)

## Types

- [DateFieldLayout](#datefieldlayout)
- [DateFieldOptions](#datefieldoptions)
- [DateField](#datefield)

## Functions

- [`getRounding()`](#getrounding)
- [`getFontPath()`](#getfontpath)

---

### DateFieldLayout

What the field puts inside itself. Auto decides from the field's own shape -- given room for the date text it shows the icon, the value and the chevron, and squarer than that it drops to the icon alone, with [DateFieldOptions](#datefieldoptions)::setLabelAspectThreshold deciding where the line falls. The three explicit modes skip the measuring and always draw the same thing.

---

### DateFieldOptions

The field's own appearance, plus the [DatePickerOptions](DatePicker.hpp.md#datepickeroptions) the popup is built from. Colours come in literal + role pairs like the rest of the library, and metrics are in virtual pixels.

---

### DateField

A date input in the shape of a dropdown: a clickable strip

```cpp
    showing the current value, which opens a DatePicker popup and
    writes back whatever is chosen. Everything the hand-wired
    version needs -- the popup, the label write-back, the hover, the
    placeholder -- is inside, so putting a date input on screen is
    one element:
datefield(Modifier().setWidth(280_px).setHeight(48_px),
          DateFieldOptions().setOnDateChanged([](const Date& d) {
              save(d);
          }))
```

No [UILO](../../UILO.hpp.md#uilo) reference and no held pointers: the field opens the popup through the [UILO](../../UILO.hpp.md#uilo) it is already bound to as part of the page.

> It adapts to the size it is given. With room for the date text it draws icon, value and chevron; squarer than setLabelAspectThreshold() it drops to just the icon, which is what makes a 40x40 field a sensible icon button and a 280x48 one a labelled field, with nothing to configure either way.

> Configure the popup through [DateFieldOptions](#datefieldoptions)::getPickerOptions(), so range mode, min/max bounds and year navigation are all reachable. In Range mode the label shows both ends.

---

### getRounding

```cpp
getRounding()
```

**Returns** — float

Corner radius of the field, resolved in three steps: the value this field was given, then the active [Theme](../../utils/Theme.hpp.md#theme)'s, then 8.

---

### getFontPath

```cpp
getFontPath()
```

**Returns** — const std::string&

[Font](../../renderer/Renderer.hpp.md#font) for the label, falling back to the active [Theme](../../utils/Theme.hpp.md#theme)'s when this field was not given one.
