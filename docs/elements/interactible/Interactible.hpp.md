# Interactible.hpp

`include/elements/interactible/Interactible.hpp`

[← index](../../README.md)

## Types

- [Interactible](#interactible)

## Functions

- [`isShortcutModifier(bool ctrl, bool gui)`](#isshortcutmodifier)

---

### Interactible

Base class for elements that carry state between clicks -- [Dropdown](Dropdown.hpp.md#dropdown), [Slider](Slider.hpp.md#slider), [Knob](Knob.hpp.md#knob), [Textbox](Textbox.hpp.md#textbox) and the like. Clicking one makes it the single active interactible through [UILO](../../UILO.hpp.md#uilo)::setCurrInteractible(); clicking a different one, or empty space, calls onDeactivate() on the previous holder so it can close itself or release focus. Keyboard and text events are routed only to the active one.

> [Button](Button.hpp.md#button) is deliberately not one of these -- it inherits [Row](../containers/Row.hpp.md#row) directly and has no open or focused state to keep.

> Every Interactible claims pointer events whether or not a [Modifier](../Modifier.hpp.md#modifier) callback was attached, so a press over a slider or a textbox is consumed rather than falling through to whatever is behind it.

---

### isShortcutModifier

```cpp
isShortcutModifier(bool ctrl, bool gui)
```

**Parameters**

- `bool ctrl`
- `bool gui`

**Returns** — bool

Whether the modifier held is the one that means "this is an editing shortcut" -- copy, cut, paste, select-all. On macOS Command and Control are accepted interchangeably, since a Mac user reaches for Command and a user coming from anywhere else reaches for Control. Elsewhere only Control counts: Super is the window manager's key, not the application's.
