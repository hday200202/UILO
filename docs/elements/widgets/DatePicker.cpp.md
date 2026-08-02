# DatePicker.cpp

`include/elements/widgets/DatePicker.cpp`

[← index](../../README.md)

## Functions

- [`DatePicker(Modifier modifier, DatePickerOptions options, const std::string& name)`](#datepicker)
- [`setOptions(const DatePickerOptions& opts)`](#setoptions)
- [`applyCardOptions()`](#applycardoptions)
- [`update(Rectf& parentBounds, float dt)`](#update)
- [`setSelectedDate(const Date& date)`](#setselecteddate)
- [`setRange(const Date& first, const Date& last)`](#setrange)
- [`clearSelection()`](#clearselection)
- [`showMonth(int year, unsigned month)`](#showmonth)
- [`updateGridInPlace(int year, unsigned month)`](#updategridinplace)
- [`nextMonth()`](#nextmonth)
- [`previousMonth()`](#previousmonth)
- [`nextYear()`](#nextyear)
- [`previousYear()`](#previousyear)
- [`goToToday()`](#gototoday)
- [`notifyMonthChanged()`](#notifymonthchanged)
- [`hasFooterButtons()`](#hasfooterbuttons)
- [`preferredHeight()`](#preferredheight)
- [`sizeToContent()`](#sizetocontent)
- [`open(UILO& uiloRef)`](#open)
- [`open()`](#open)
- [`close()`](#close)
- [`clearRows()`](#clearrows)
- [`rebuild()`](#rebuild)
- [`buildHeader()`](#buildheader)
- [`buildNavButton(const std::string& iconName, std::function&lt;void()&gt; action)`](#buildnavbutton)
- [`buildWeekdayHeader()`](#buildweekdayheader)
- [`buildGrid()`](#buildgrid)
- [`buildDayCell(const Date& date, bool adjacent)`](#builddaycell)
- [`buildFooter()`](#buildfooter)
- [`buildFooterButton(...)`](#buildfooterbutton)
- [`isSelectable(const Date& date)`](#isselectable)
- [`isInRange(const Date& date)`](#isinrange)
- [`isRangeEndpoint(const Date& date)`](#israngeendpoint)
- [`applyCellColors()`](#applycellcolors)
- [`setHoverDate(const std::optional&lt;Date&gt;& date)`](#sethoverdate)
- [`handleDayClicked(const Date& date, bool adjacent)`](#handledayclicked)

---

### DatePicker

```cpp
DatePicker(Modifier modifier, DatePickerOptions options, const std::string& name)
```

**Parameters**

- `Modifier modifier`
- `DatePickerOptions options`
- `const std::string& name`

**Returns** — [DatePicker](DatePicker.hpp.md#datepicker)

Builds the picker as a [Column](../containers/Column.hpp.md#column): header, weekday row, day grid and footer. The modifier is kept as well as applied, because opening the picker as a popup replaces it with a centred free position and closing has to put the embedded one back.

---

### setOptions

```cpp
setOptions(const DatePickerOptions& opts)
```

**Parameters**

- `const DatePickerOptions& opts`

**Returns** — void

Swaps in a new configuration, re-applies the card surface, and queues a grid rebuild for the next update.

---

### applyCardOptions

```cpp
applyCardOptions()
```

**Returns** — void

Pushes the background and rounding down onto the [Column](../containers/Column.hpp.md#column) the card is built from.

---

### update

```cpp
update(Rectf& parentBounds, float dt)
```

**Parameters**

- `Rectf& parentBounds`
- `float dt`

**Returns** — void

Rebuilds the grid first if something queued one, then lays out as a [Column](../containers/Column.hpp.md#column). Rebuilds are deferred to here because clicks are dispatched while the parent walks its child list, and mutating that list from a click handler would invalidate the walk in progress.

---

### setSelectedDate

```cpp
setSelectedDate(const Date& date)
```

**Parameters**

- `const Date& date`

**Returns** — void

Selects a date and pages the grid to its month. Fires no callback: this is the application driving the widget, and an application that wanted to hear about it already knows.

---

### setRange

```cpp
setRange(const Date& first, const Date& last)
```

**Parameters**

- `const Date& first`
- `const Date& last`

**Returns** — void

Sets both ends of a range, in either order, and pages to the start. Fires no callback, as with setSelectedDate().

---

### clearSelection

```cpp
clearSelection()
```

**Returns** — void

Drops the selection and any half-made range, and fires onCleared.

---

### showMonth

```cpp
showMonth(int year, unsigned month)
```

**Parameters**

- `int year`
- `unsigned month`

**Returns** — void

Pages the grid to a month, ignoring one outside 1-12. Queues a rebuild rather than doing it here, so this is safe to call from a click handler.

---

### updateGridInPlace

```cpp
updateGridInPlace(int year, unsigned month)
```

**Parameters**

- `int year`
- `unsigned month`

**Returns** — bool -- true if the grid was paged in place, false if the shape differs between months and a rebuild is required instead.

Rewrites each existing day cell to the day it now shows -- its number, whether it belongs to the month, and the date its click and hover handlers carry -- then refreshes the title and recolours. No element is created or destroyed, so the Wt bridge picks the changes up through its ordinary property sync.

---

### nextMonth

```cpp
nextMonth()
```

**Returns** — void

Pages forward one month, carrying into the next year.

---

### previousMonth

```cpp
previousMonth()
```

**Returns** — void

Pages back one month, carrying into the previous year.

---

### nextYear

```cpp
nextYear()
```

**Returns** — void

Pages forward twelve months.

---

### previousYear

```cpp
previousYear()
```

**Returns** — void

Pages back twelve months.

---

### goToToday

```cpp
goToToday()
```

**Returns** — void

Pages to the current month without touching the selection.

---

### notifyMonthChanged

```cpp
notifyMonthChanged()
```

**Returns** — void

Fires onMonthChanged for the month now on show.

---

### hasFooterButtons

```cpp
hasFooterButtons()
```

**Returns** — bool

Whether the footer would draw anything: showFooter on and at least one label non-empty. Both preferredHeight() and buildFooter() go through this so the height reserved and the row actually built can never disagree.

---

### preferredHeight

```cpp
preferredHeight()
```

**Returns** — float

The height the configured metrics add up to, in virtual pixels: padding, header, weekday strip, six week rows (or as many as the month needs when fixed rows are off), and the footer. What open() uses when no explicit popup height is set, and what sizeToContent() writes into the modifier.

---

### sizeToContent

```cpp
sizeToContent()
```

**Returns** — [DatePicker](DatePicker.hpp.md#datepicker)*

Fixes the picker's height at preferredHeight() so an embedded one takes exactly the room it needs instead of stretching to its slot. Returns this, to be used inline where the element is declared:

```cpp
datepicker(Modifier(), opts)->sizeToContent()
```

With fixed week rows (the default) one call holds for every month. With them off the height follows the month on show, so call it again from onMonthChanged to keep it tight.

---

### open

```cpp
open(UILO& uiloRef)
```

**Parameters**

- `UILO& uiloRef`

**Returns** — void

Presents the picker centred over the window on a dimmed backdrop. The backdrop goes in as a floating element rather than an overlay because [UILO](../../UILO.hpp.md#uilo) ticks and renders floating elements every frame but leaves overlays to whoever owns them -- and a popup picker, being outside the page tree, has no owner to do that. Clicks land on the backdrop first, which is what makes the popup modal.

---

### open

```cpp
open()
```

**Returns** — void

Opens against the [UILO](../../UILO.hpp.md#uilo) the picker already knows -- one it was opened with before, or inherited from a parent. No-op before either has happened, since there is nothing to present on.

---

### close

```cpp
close()
```

**Returns** — void

Takes the backdrop back out of the floating list and restores the modifier the picker was constructed with, so a picker that is also used embedded goes back to its layout geometry. Fires onClosed.

---

### clearRows

```cpp
clearRows()
```

**Returns** — void

Tears down the whole grid and forgets the per-cell tracking arrays.

---

### rebuild

```cpp
rebuild()
```

**Returns** — void

Builds the card contents for the month on show: header, weekday strip, day grid, footer. Only paging months and changing options come through here -- selecting and hovering go through applyCellColors(), which touches no allocation.

---

### buildHeader

```cpp
buildHeader()
```

**Returns** — [Element](../Element.hpp.md#element)*

The title strip: optional year arrows, month arrows, and the month name filling the space between them.

---

### buildNavButton

```cpp
buildNavButton(const std::string& iconName, std::function<void()> action)
```

**Parameters**

- `const std::string& iconName`
- `std::function<void()> action`

**Returns** — [Element](../Element.hpp.md#element)*

A square icon button. A [Row](../containers/Row.hpp.md#row) rather than a [Button](../interactible/Button.hpp.md#button) because [ButtonOptions](../interactible/Button.hpp.md#buttonoptions) only takes a [Text](../decoration/Text.hpp.md#text) label, and these carry icons.

---

### buildWeekdayHeader

```cpp
buildWeekdayHeader()
```

**Returns** — [Element](../Element.hpp.md#element)*

Seven column headings, starting from the configured first day of the week. Split the same way as the grid below so the letters line up over their columns.

---

### buildGrid

```cpp
buildGrid()
```

**Returns** — [Element](../Element.hpp.md#element)*

The day cells, as a [Column](../containers/Column.hpp.md#column) of week Rows. Every row holds seven cells whatever the month, walking whole weeks from gridStart() so the columns stay aligned to their weekdays; cells outside the month are marked adjacent and styled apart.

---

### buildDayCell

```cpp
buildDayCell(const Date& date, bool adjacent)
```

**Parameters**

- `const Date& date`
- `bool adjacent`

**Returns** — [Element](../Element.hpp.md#element)*

One day: a [Row](../containers/Row.hpp.md#row) carrying the fill and the click, with the day number centred in it. Colours are left to applyCellColors(), which runs immediately after the grid is assembled and again on every selection or hover change.

---

### buildFooter

```cpp
buildFooter()
```

**Returns** — [Element](../Element.hpp.md#element)*

The action row: Today and Clear on the left, Cancel and the confirm button on the right. Returns nothing when every label is empty, so an all-empty footer takes up no space.

---

### buildFooterButton

```cpp
buildFooterButton(...)
```

**Parameters**

- `const std::string& label`
- `bool primary`
- `std::function<void()> action`

**Returns** — [Element](../Element.hpp.md#element)*

One footer button. The primary one takes the confirm colours, which are the accent by default so the committing action reads as the committing action.

---

### isSelectable

```cpp
isSelectable(const Date& date)
```

**Parameters**

- `const Date& date`

**Returns** — bool

Whether a day can be picked: a real date, inside any min/max bounds, and passed by the dateEnabled predicate.

---

### isInRange

```cpp
isInRange(const Date& date)
```

**Parameters**

- `const Date& date`

**Returns** — bool

True for a day strictly between the ends of a range. While the second end is still missing the cell under the cursor stands in for it, which is what previews the span as the pointer moves.

---

### isRangeEndpoint

```cpp
isRangeEndpoint(const Date& date)
```

**Parameters**

- `const Date& date`

**Returns** — bool

True at either end of a range, the previewed end included.

---

### applyCellColors

```cpp
applyCellColors()
```

**Returns** — void

Re-derives every visible cell's fill, text colour, and weight from the current state, in priority order: disabled, then selected or a range endpoint, then inside a range, then today, then hover, then a plain day. Mutating the existing options rather than rebuilding is what lets selection and hover cost nothing.

---

### setHoverDate

```cpp
setHoverDate(const std::optional<Date>& date)
```

**Parameters**

- `const std::optional<Date>& date`

**Returns** — void

Tracks which day the cursor is over and restyles the grid when that changes. The hover fill could be done per cell, but a half- made range has to repaint the whole previewed span, which only the grid as a whole can do.

---

### handleDayClicked

```cpp
handleDayClicked(const Date& date, bool adjacent)
```

**Parameters**

- `const Date& date`
- `bool adjacent`

**Returns** — void

Applies a click on a day cell. Single mode replaces the selection outright; Range mode opens a span on the first click and closes it on the second. A click on a neighbour-month day pages the view to that month, so the picked date is never left off screen.
