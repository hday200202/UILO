# ContextMenu.hpp

`include/elements/widgets/ContextMenu.hpp`

[← index](../../README.md)

## Types

- [ContextMenuOptions](#contextmenuoptions)
- [ContextMenu](#contextmenu)

---

### ContextMenuOptions

How a context menu is drawn. Every colour defaults to a palette role, so a menu follows the application's theme with nothing named at the call site.

---

### ContextMenu

The popup itself: a [Column](../containers/Column.hpp.md#column) of rows, sized to its widest label and placed at the cursor. [UILO](../../UILO.hpp.md#uilo) owns one at the root of the UI and fills it from whichever element was right-clicked, so an application never builds or positions one -- it declares items with [Modifier](../Modifier.hpp.md#modifier)::setContextMenu and this is what shows them.

> Rows are pooled. setItems() relabels and shows the slots it needs and hides the rest, and only ever grows the pool, because the web bridge translates structure once per session and would not see a rebuild. Each slot holds both an item row and a separator rule, and shows whichever the item at that position is.

> A submenu is another ContextMenu, created on demand and owned by the parent one. It is registered as its own overlay so it is hit-tested above its parent, and closed whenever the highlight moves off the row that opened it.
