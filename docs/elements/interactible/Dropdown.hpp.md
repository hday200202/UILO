# Dropdown.hpp

`include/elements/interactible/Dropdown.hpp`

[← index](../../README.md)

## Types

- [DropdownOptions](#dropdownoptions)
- [Dropdown](#dropdown)

---

### DropdownOptions

Everything a [Dropdown](#dropdown) draws, in three groups: the header the user clicks, the popup that opens under it, and the items inside that popup. Colors come as a literal plus a role, where the role wins when it resolves against the active [Palette](../../Palette.hpp.md#palette) and the literal is the fallback.

> setMaxItems caps how tall the popup grows; beyond that it scrolls rather than running off the window.

---

### Dropdown

A header button that opens a list of items. The header is a [Button](Button.hpp.md#button) and the popup a scrollable [Column](../containers/Column.hpp.md#column), both built once at construction and kept for the element's lifetime -- opening and closing only adds and removes the popup from [UILO](../../UILO.hpp.md#uilo)'s floating layer, so no element is created or destroyed on a click.

> It is an [Element](../Element.hpp.md#element) rather than an [Interactible](Interactible.hpp.md#interactible) because the header does the claiming: the press is consumed there whatever callbacks are attached.

> The popup has to be floating rather than a child, because [UILO](../../UILO.hpp.md#uilo) only ticks, hit-tests and renders the floating layer above the page -- a popup left in the tree would be clipped by whatever container the dropdown sits in.

> On the web this becomes a native <select>, so the popup and the arrow are never translated; getItems() exists so that backend can populate its own control from the whole list rather than just the selection.
