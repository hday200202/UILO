# DateField.cpp

`include/elements/widgets/DateField.cpp`

[← index](../../README.md)

## Functions

- [`DateField(Modifier modifier, DateFieldOptions options, const std::string& name)`](#datefield)
- [`setOptions(const DateFieldOptions& opts)`](#setoptions)
- [`applySurface()`](#applysurface)
- [`update(Rectf& parentBounds, float dt)`](#update)
- [`resolveAspect()`](#resolveaspect)
- [`resolveLayout()`](#resolvelayout)
- [`applyLayout(DateFieldLayout layout)`](#applylayout)
- [`getDisplayText()`](#getdisplaytext)
- [`refreshLabel()`](#refreshlabel)
- [`setDate(const Date& date)`](#setdate)
- [`setRange(const Date& first, const Date& last)`](#setrange)
- [`clear()`](#clear)
- [`handlePicked(const Date& date)`](#handlepicked)
- [`handleRangePicked(const Date& first, const Date& last)`](#handlerangepicked)
- [`open()`](#open)
- [`close()`](#close)
- [`isOpen()`](#isopen)

---

### DateField

```cpp
DateField(Modifier modifier, DateFieldOptions options, const std::string& name)
```

**Parameters**

- `Modifier modifier`
- `DateFieldOptions options`
- `const std::string& name`

**Returns** — [DateField](DateField.hpp.md#datefield)

Builds the field as a [Row](../containers/Row.hpp.md#row) and creates every part it will ever need -- the insets, the icon, the gap, the label and the chevron -- whatever shape it is currently in. Parts are only ever shown or hidden from then on, never created or destroyed, because the web bridge translates structure once and would never see a rebuild.

---

### setOptions

```cpp
setOptions(const DateFieldOptions& opts)
```

**Parameters**

- `const DateFieldOptions& opts`

**Returns** — void

Swaps in a new configuration and re-applies everything derived from it. The popup is left alone -- it was built from the options given at construction, and getPicker() is how to change it afterwards.

---

### applySurface

```cpp
applySurface()
```

**Returns** — void

Pushes the background and rounding down onto the [Row](../containers/Row.hpp.md#row), and the metrics onto the parts that carry them.

---

### update

```cpp
update(Rectf& parentBounds, float dt)
```

**Parameters**

- `Rectf& parentBounds`
- `float dt`

**Returns** — void

Resolves the field's own box first so the layout decision is made against this frame's size rather than last frame's, then lays out as a [Row](../containers/Row.hpp.md#row). Native only: the web bridge does not tick, and takes the decision made in the constructor.

---

### resolveAspect

```cpp
resolveAspect()
```

**Returns** — float

The field's width over its height. Prefers the laid-out box, which is exact and follows percent sizing, and falls back to the declared width and height when there is none -- either before the first frame or under the web bridge, which never lays the tree out. Returns 0 when the field is sized in percentages and has not been laid out, since nothing available can say.

---

### resolveLayout

```cpp
resolveLayout()
```

**Returns** — [DateFieldLayout](DateField.hpp.md#datefieldlayout)

Turns Auto into a concrete mode. Wide enough for the text and it keeps the label; squarer than the threshold and it drops to the icon. An unknown aspect resolves to the labelled form, which degrades better -- a too-narrow label is clipped, where a needlessly bare icon hides the value outright.

---

### applyLayout

```cpp
applyLayout(DateFieldLayout layout)
```

**Parameters**

- `DateFieldLayout layout`

**Returns** — void

Shows the parts the mode calls for and hides the rest. Nothing is created or destroyed: visibility is a style the web bridge re-applies on every sync, where a change to the child list would never be seen. Returns early when the mode has not changed, so this costs a comparison per frame.

---

### getDisplayText

```cpp
getDisplayText()
```

**Returns** — std::string

What the label reads: the formatted value, both ends joined when a range is set, or the placeholder while the field is empty.

---

### refreshLabel

```cpp
refreshLabel()
```

**Returns** — void

Writes the current text into the label and colours it as a value or as a placeholder. Both are ordinary property changes, so this reaches the web through the bridge's usual sync.

---

### setDate

```cpp
setDate(const Date& date)
```

**Parameters**

- `const Date& date`

**Returns** — void

Sets the value and pages the popup to it. Fires no callback: this is the application writing the field, not the user picking.

---

### setRange

```cpp
setRange(const Date& first, const Date& last)
```

**Parameters**

- `const Date& first`
- `const Date& last`

**Returns** — void

Sets both ends of a range, in either order. Silent, as setDate() is.

---

### clear

```cpp
clear()
```

**Returns** — void

Empties the field, returning the label to the placeholder, and fires onCleared.

---

### handlePicked

```cpp
handlePicked(const Date& date)
```

**Parameters**

- `const Date& date`

**Returns** — void

A date came back from the popup. In Range mode this is the first of the two clicks, so the end is dropped until the span closes and the label shows the start on its own meanwhile.

---

### handleRangePicked

```cpp
handleRangePicked(const Date& first, const Date& last)
```

**Parameters**

- `const Date& first`
- `const Date& last`

**Returns** — void

Both ends of a range arrived; the label switches to the joined form and onRangeChanged fires.

---

### open

```cpp
open()
```

**Returns** — void

Opens the calendar over the window, through the [UILO](../../UILO.hpp.md#uilo) the field was bound to when it became part of a page. No-op before then, since there is nothing to present on.

---

### close

```cpp
close()
```

**Returns** — void

Dismisses the calendar. onClosed fires from the picker, so it reports a dismissal however it came about.

---

### isOpen

```cpp
isOpen()
```

**Returns** — bool

Whether the calendar is currently up.
