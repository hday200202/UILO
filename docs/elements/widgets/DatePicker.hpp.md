# DatePicker.hpp

`include/elements/widgets/DatePicker.hpp`

[← index](../../README.md)

## Types

- [DatePickerMode](#datepickermode)
- [WeekdayLabelStyle](#weekdaylabelstyle)
- [DatePickerOptions](#datepickeroptions)
- [DatePicker](#datepicker)

## Functions

- [`getRounding()`](#getrounding)
- [`getFontPath()`](#getfontpath)
- [`getNavRounding()`](#getnavrounding)
- [`getCellRounding()`](#getcellrounding)
- [`getButtonRounding()`](#getbuttonrounding)

---

### DatePickerMode

Single takes one date at a time, so every click replaces the selection. Range brackets a span with two clicks -- the first starts it, the second closes it, and a third begins a new one -- and while a range is half made the cells under the cursor preview where it would land.

---

### WeekdayLabelStyle

How the column headings above the grid are written. Initial ("S") suits a narrow picker, Short ("Sun") a wide one.

---

### DatePickerOptions

Everything the widget draws, in the same shape as the rest of the library: literal + role colour pairs where the role wins if it resolves against the active [Palette](../../Palette.hpp.md#palette), and plain pixel metrics in virtual units.

> Sizes are all "before scale": [UILO](../../UILO.hpp.md#uilo) multiplies by getScale() at layout time, so a picker configured once is the right physical size on any display.

---

### DatePicker

A month grid for choosing a date, built on [Column](../containers/Column.hpp.md#column). Header with month title and arrows, weekday headings, six rows of day cells, and an optional button row.

> There are two ways to use one. Embedded -- add it to a layout like any other element and read the selection from the callbacks.

```cpp
column(Modifier(), {}, contains{
    datepicker(Modifier().setHeight(320_px),
               DatePickerOptions().setOnDateSelected(...))
})
```

Centred popup -- keep the pointer, and open it when something asks for a date. The picker builds its own full-window backdrop, centres itself on it, and blocks the page beneath until dismissed.

```cpp
auto* picker = datepicker({}, DatePickerOptions()
    .setCloseOnSelect(true)
    .setOnDateSelected([](const Date& d) { ... }));
...
picker->open(ui);
```

A popup picker must not also be a child of the page tree; open() makes it a child of its own backdrop, and an element cannot be in two places.

> Paging months rebuilds the grid. Selecting, hovering, and previewing a range only recolour the existing cells, so dragging across the grid allocates nothing.

---

### getRounding

```cpp
getRounding()
```

**Returns** — float

Corner radius of the picker card, resolved in three steps: the value this picker was given, then the active [Theme](../../utils/Theme.hpp.md#theme)'s, then 10.

---

### getFontPath

```cpp
getFontPath()
```

**Returns** — const std::string&

[Font](../../renderer/Renderer.hpp.md#font) for every label in the picker, falling back to the active [Theme](../../utils/Theme.hpp.md#theme)'s.

---

### getNavRounding

```cpp
getNavRounding()
```

**Returns** — float

Corner radius of the month and year navigation buttons, resolved the same three ways with a fallback of 6.

---

### getCellRounding

```cpp
getCellRounding()
```

**Returns** — float

Corner radius of a day cell, resolved the same three ways with a fallback of 6.

---

### getButtonRounding

```cpp
getButtonRounding()
```

**Returns** — float

Corner radius of the footer buttons, resolved the same three ways with a fallback of 6.
