# Dropdown.cpp

`include/elements/interactible/Dropdown.cpp`

[← index](../../README.md)

## Functions

- [`setUILO(UILO& uiloRef)`](#setuilo)
- [`updateArrowIcon()`](#updatearrowicon)
- [`updateHeaderLabel()`](#updateheaderlabel)
- [`setSelectedIndex(int idx)`](#setselectedindex)
- [`getSelectedItem()`](#getselecteditem)
- [`computePopupBounds()`](#computepopupbounds)
- [`openPopup()`](#openpopup)
- [`closePopup()`](#closepopup)
- [`update(Rectf& parentBounds, float dt)`](#update)
- [`render()`](#render)
- [`checkLeftClick(const Vec2f& mousePosition)`](#checkleftclick)
- [`checkHover(const Vec2f& mousePosition)`](#checkhover)

---

### setUILO

```cpp
setUILO(UILO& uiloRef)
```

**Parameters**

- `UILO& uiloRef`

**Returns** — void

Binds the dropdown and everything it built to the owning [UILO](../../UILO.hpp.md#uilo), so the header, the popup and all their children land in the element pool. The popup is registered here even though it is not a child: it has to be owned before it can ever be shown, and it is only added to the floating layer when opened.

---

### updateArrowIcon

```cpp
updateArrowIcon()
```

**Returns** — void

Points the arrow at the open glyph while the popup is up and back at the closed one when it is not. An empty open icon leaves it alone, so the arrow can be made to stay put.

---

### updateHeaderLabel

```cpp
updateHeaderLabel()
```

**Returns** — void

Puts the selected item's text on the header, or the placeholder when nothing is selected yet.

---

### setSelectedIndex

```cpp
setSelectedIndex(int idx)
```

**Parameters**

- `int idx`

**Returns** — void

Selects an item by position, updates the header, and fires onItemChanged. An index outside the list clears the selection back to the placeholder rather than being ignored.

---

### getSelectedItem

```cpp
getSelectedItem()
```

**Returns** — const std::string& -- empty when nothing is selected

The text of the selected item.

---

### computePopupBounds

```cpp
computePopupBounds()
```

**Returns** — [Rectf](../../utils/Math.hpp.md#rectf)

Where the popup sits: directly under the header, matching its width, and as tall as the items need up to the maxItems cap. It is flipped above the header when there is not enough room below, so a dropdown near the bottom of the window still shows its list.

---

### openPopup

```cpp
openPopup()
```

**Returns** — void

Shows the list by adding the popup to [UILO](../../UILO.hpp.md#uilo)'s floating layer, which is what gets it ticked, hit-tested and drawn above the page instead of clipped by whatever container the dropdown sits in. The popup itself already exists, so this only changes where it is registered.

---

### closePopup

```cpp
closePopup()
```

**Returns** — void

Hides the list by removing the popup from the floating layer and points the arrow back at its closed glyph. The popup is kept rather than destroyed, so reopening costs nothing and the web bridge -- which only translates structure once -- is not invalidated.

---

### update

```cpp
update(Rectf& parentBounds, float dt)
```

**Parameters**

- `Rectf& parentBounds`
- `float dt`

**Returns** — void

Resolves the dropdown's bounds, ticks the header with them, and ticks the popup against its computed bounds while it is open. The [Modifier](../Modifier.hpp.md#modifier)'s material is mirrored onto both, so a glass dropdown looks the same collapsed and open. Also handles dismissal: a click outside closes the popup, and the just- dismissed flag stops that same click from immediately reopening it through the header.

---

### render

```cpp
render()
```

**Returns** — void

Draws the header. The popup is not drawn here: it is in the floating layer, which [UILO](../../UILO.hpp.md#uilo) renders after the page so the list appears above everything rather than under a later sibling.

---

### checkLeftClick

```cpp
checkLeftClick(const Vec2f& mousePosition)
```

**Parameters**

- `const Vec2f& mousePosition`

**Returns** — bool -- true when the click landed on the header

Toggles the popup. A click that just dismissed an open popup is swallowed rather than treated as a fresh open, so clicking the header of an open dropdown closes it instead of closing and reopening in the same frame.

---

### checkHover

```cpp
checkHover(const Vec2f& mousePosition)
```

**Parameters**

- `const Vec2f& mousePosition`

**Returns** — bool -- true when the pointer is over the header or the open popup

Forwards hover to the header and, while open, to the popup's items, so an item highlights under the pointer even though the popup is not a child of this element.
