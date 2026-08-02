# Filebrowser.hpp

`include/elements/widgets/Filebrowser.hpp`

[← index](../../README.md)

## Types

- [FileBrowserSort](#filebrowsersort)
- [FileBrowserOptions](#filebrowseroptions)
- [FileBrowser](#filebrowser)

## Functions

- [`setIconForKind(FileKind kind, std::string_view n)`](#seticonforkind)
- [`getIconForKind(FileKind kind)`](#geticonforkind)
- [`getRounding()`](#getrounding)
- [`getEntryRounding()`](#getentryrounding)
- [`getHeaderRounding()`](#getheaderrounding)
- [`getFontPath()`](#getfontpath)
- [`getDirectoryArrowBlockWidth()`](#getdirectoryarrowblockwidth)

---

### FileBrowserSort

Ordering applied to the entries of each directory. Size and Modified stat each file the first time a directory is listed; Name and Extension are free, since they only read the cached path.

---

### FileBrowserOptions

Everything the widget draws is configurable here. Colors come in literal plus role pairs like the rest of the library: the role wins when it resolves against the active [Palette](../../Palette.hpp.md#palette), otherwise the literal is used. Sizes are unscaled content pixels; [UILO](../../UILO.hpp.md#uilo) multiplies by its scale at layout time.

---

### FileBrowser

A scrollable directory listing built on top of [Column](../containers/Column.hpp.md#column). Each visible entry is a [Row](../containers/Row.hpp.md#row) -- background plus hover and selection state -- holding an indent [Spacer](../decoration/Spacer.hpp.md#spacer), an icon, a [Text](../decoration/Text.hpp.md#text) label, and for a directory an arrow pinned to the right edge. Directories expand and collapse on click, one level at a time: the underlying [FileTree](../../utils/FileTree.hpp.md#filetree) only lists a directory the first time it is opened and caches it from then on, so deep trees stay cheap. Expansion state is keyed by path rather than stored on the tree nodes, so refresh() can re-scan the disk without losing what the user opened.

---

### setIconForKind

```cpp
setIconForKind(FileKind kind, std::string_view n)
```

**Parameters**

- `FileKind kind`
- `std::string_view n`

**Returns** — [FileBrowserOptions](#filebrowseroptions)&

Overrides the icon used for one [FileKind](../../utils/FileTree.hpp.md#filekind), so a project can give its own file types a recognisable glyph without turning the whole kind mapping off.

---

### getIconForKind

```cpp
getIconForKind(FileKind kind)
```

**Parameters**

- `FileKind kind`

**Returns** — const std::string&

The icon name registered for a [FileKind](../../utils/FileTree.hpp.md#filekind). Empty when that kind was deliberately cleared, which sends the caller back to setFileIcon().

---

### getRounding

```cpp
getRounding()
```

**Returns** — float

Corner radius of the panel, resolved in three steps: the value this widget was given, then the active Theme's, then 8. Resolved on every read rather than cached, so changing the Theme restyles a browser already on screen.

---

### getEntryRounding

```cpp
getEntryRounding()
```

**Returns** — float

Corner radius of one entry row, resolved the same three ways as getRounding with a fallback of 4.

---

### getHeaderRounding

```cpp
getHeaderRounding()
```

**Returns** — float

Corner radius of the pinned header strip, resolved the same three ways as getRounding with a fallback of 0 -- square, since the header spans the full width of the panel.

---

### getFontPath

```cpp
getFontPath()
```

**Returns** — const std::string&

[Font](../../renderer/Renderer.hpp.md#font) for the entry labels, falling back to the active Theme's when this widget was not given one.

---

### getDirectoryArrowBlockWidth

```cpp
getDirectoryArrowBlockWidth()
```

**Returns** — float

Total width the arrow occupies at the right end of a row: the gap ahead of it, the arrow itself, and the inset behind it. A file row reserves exactly this much so its label clips where a directory's does.
