# ContextMenuItem.hpp

`include/utils/ContextMenuItem.hpp`

[← index](../README.md)

## Types

- [ContextMenuItem](#contextmenuitem)
- [ContextMenuBuilder](#contextmenubuilder)

## Functions

- [`menuItem(std::string label, std::function&lt;void()&gt; action)`](#menuitem)
- [`menuItem(std::string label, std::string icon, std::function&lt;void()&gt; action)`](#menuitem)
- [`menuSeparator()`](#menuseparator)
- [`menuSubmenu(std::string label, std::vector&lt;ContextMenuItem&gt; children)`](#menusubmenu)
- [`menuSubmenu(std::string label, std::string icon, std::vector&lt;ContextMenuItem&gt; children)`](#menusubmenu)

---

### ContextMenuItem

One line of a context menu: a label, an optional icon, and either an action to run or a submenu to open. A separator is an item with nothing on it but the flag.

> Kept free of every element type on purpose. [Modifier](../elements/Modifier.hpp.md#modifier) stores these, and [Modifier](../elements/Modifier.hpp.md#modifier) is included by everything, so this header has to depend on nothing but the standard library.

> Build them with menuItem(), menuSeparator() and menuSubmenu() rather than filling the struct in by hand.

---

### ContextMenuBuilder

What [Modifier](../elements/Modifier.hpp.md#modifier) stores: a callable that produces the items. The list form of setContextMenu wraps a fixed vector in one of these, so there is a single code path and a menu whose contents depend on application state costs nothing extra.

---

### menuItem

```cpp
menuItem(std::string label, std::function<void()> action)
```

**Parameters**

- `std::string label`
- `std::function<void()> action`

**Returns** — [ContextMenuItem](#contextmenuitem)

An ordinary item that runs `action` when it is picked.

---

### menuItem

```cpp
menuItem(std::string label, std::string icon, std::function<void()> action)
```

**Parameters**

- `std::string label`
- `std::string_view icon`
- `std::function<void()> action`

**Returns** — [ContextMenuItem](#contextmenuitem)

An item with an icon ahead of its label. Every item in a menu shares one icon column, so one item having an icon indents the labels of the rest to match.

---

### menuSeparator

```cpp
menuSeparator()
```

**Returns** — [ContextMenuItem](#contextmenuitem)

A dividing rule. It takes a row of its own but cannot be highlighted, so keyboard navigation steps over it.

---

### menuSubmenu

```cpp
menuSubmenu(std::string label, std::vector<ContextMenuItem> children)
```

**Parameters**

- `std::string label`
- `std::vector<ContextMenuItem> children`

**Returns** — [ContextMenuItem](#contextmenuitem)

An item that opens a nested menu beside itself instead of running an action. Nests to any depth.

---

### menuSubmenu

```cpp
menuSubmenu(std::string label, std::string icon, std::vector<ContextMenuItem> children)
```

**Parameters**

- `std::string label`
- `std::string_view icon`
- `std::vector<ContextMenuItem> children`

**Returns** — [ContextMenuItem](#contextmenuitem)

menuSubmenu with an icon ahead of the label.
