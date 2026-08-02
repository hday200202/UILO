# Filebrowser.cpp

`include/elements/widgets/Filebrowser.cpp`

[← index](../../README.md)

## Functions

- [`panelOptionsFrom(const FileBrowserOptions& o)`](#paneloptionsfrom)
- [`toLower(std::string s)`](#tolower)
- [`eraseSubtree(Element* element)`](#erasesubtree)
- [`FileBrowser(Modifier modifier, FileBrowserOptions options, const std::string& name)`](#filebrowser)
- [`setOptions(const FileBrowserOptions& opts)`](#setoptions)
- [`applyPanelOptions()`](#applypaneloptions)
- [`update(Rectf& parentBounds, float dt)`](#update)
- [`setRootPath(const fs::path& path)`](#setrootpath)
- [`getRootPath()`](#getrootpath)
- [`refresh()`](#refresh)
- [`isExpanded(const fs::path& path)`](#isexpanded)
- [`expandPath(const fs::path& path)`](#expandpath)
- [`collapsePath(const fs::path& path)`](#collapsepath)
- [`togglePath(const fs::path& path)`](#togglepath)
- [`collapseAll()`](#collapseall)
- [`setSelectedPath(const fs::path& path)`](#setselectedpath)
- [`clearSelection()`](#clearselection)
- [`handleEntryClicked(const fs::path& path, bool isDirectory)`](#handleentryclicked)
- [`clearRows()`](#clearrows)
- [`rebuildRows()`](#rebuildrows)
- [`rootDisplayName()`](#rootdisplayname)
- [`buildHeader()`](#buildheader)
- [`appendDirectory(Directory* dir, int depth)`](#appenddirectory)
- [`passesFilter(const FSEntry* entry)`](#passesfilter)
- [`sortEntries(std::vector&lt;FSEntry*&gt;& entries)`](#sortentries)
- [`appendEntryIcon(...)`](#appendentryicon)
- [`appendDirectoryArrow(...)`](#appenddirectoryarrow)
- [`buildRow(FSEntry* entry, int depth)`](#buildrow)
- [`addEntryRow(Element* row)`](#addentryrow)
- [`iconNameFor(const FSEntry* entry)`](#iconnamefor)

---

### panelOptionsFrom

```cpp
panelOptionsFrom(const FileBrowserOptions& o)
```

**Parameters**

- `const FileBrowserOptions& o`

**Returns** — [ColumnOptions](../containers/Column.hpp.md#columnoptions)

Builds the options for the panel the entries sit in. The panel is a plain [Column](../containers/Column.hpp.md#column), and [FileBrowserOptions](Filebrowser.hpp.md#filebrowseroptions) owns the subset of [ColumnOptions](../containers/Column.hpp.md#columnoptions) that makes sense to expose, so the widget keeps a single Options type in its public API. Rounding is passed through unresolved so the panel keeps following the Theme.

---

### toLower

```cpp
toLower(std::string s)
```

**Parameters**

- `std::string s`

**Returns** — std::string

ASCII lowercase, for case-insensitive sorting and extension matching.

---

### eraseSubtree

```cpp
eraseSubtree(Element* element)
```

**Parameters**

- `Element* element`

**Returns** — void

Marks an element and everything below it for deletion. Erasing only the row would leave its [Spacer](../decoration/Spacer.hpp.md#spacer), [Icon](../decoration/Icon.hpp.md#icon) and [Text](../decoration/Text.hpp.md#text) in [UILO](../../UILO.hpp.md#uilo)'s element pool for the lifetime of the program, since the pool sweep is driven by the flag alone. An element that was never registered with a [UILO](../../UILO.hpp.md#uilo) -- rebuilt before the page walk reached it -- has no other owner, so it is deleted outright instead.

---

### FileBrowser

```cpp
FileBrowser(Modifier modifier, FileBrowserOptions options, const std::string& name)
```

**Parameters**

- `Modifier modifier`
- `FileBrowserOptions options`
- `const std::string& name`

**Returns** — [FileBrowser](Filebrowser.hpp.md#filebrowser)

Constructs the browser as a [Column](../containers/Column.hpp.md#column) carrying the panel options, opens the tree at the configured root when one was given, and builds the first set of entry rows.

---

### setOptions

```cpp
setOptions(const FileBrowserOptions& opts)
```

**Parameters**

- `const FileBrowserOptions& opts`

**Returns** — void

Replaces the options, re-applies the panel settings and schedules a rebuild of the entry rows. A changed root path is handled by setRootPath instead, which also clears expansion and selection.

---

### applyPanelOptions

```cpp
applyPanelOptions()
```

**Returns** — void

Pushes the panel half of the widget's options down onto the [Column](../containers/Column.hpp.md#column) it is built from.

---

### update

```cpp
update(Rectf& parentBounds, float dt)
```

**Parameters**

- `Rectf& parentBounds`
- `float dt`

**Returns** — void

Rebuilds the entry rows when one is pending, then lays out as a [Column](../containers/Column.hpp.md#column). The rebuild happens here rather than at the point that asked for it because clicks are dispatched while the parent walks its child list, and mutating that list from a click handler would invalidate the iteration in progress.

---

### setRootPath

```cpp
setRootPath(const fs::path& path)
```

**Parameters**

- `const fs::path& path`

**Returns** — void

Points the browser at a new root, clearing expansion and selection since neither means anything under a different tree. An empty path drops the tree entirely, leaving the panel blank.

---

### getRootPath

```cpp
getRootPath()
```

**Returns** — fs::path

The root the tree is currently open at, empty when there is no tree.

---

### refresh

```cpp
refresh()
```

**Returns** — void

Re-scans the disk, keeping expansion state. Every cached listing is dropped; the directories still in the expanded set are re- listed lazily as the rows are rebuilt, so what the user had open comes back.

---

### isExpanded

```cpp
isExpanded(const fs::path& path)
```

**Parameters**

- `const fs::path& path`

**Returns** — bool

Whether a directory is currently open.

---

### expandPath

```cpp
expandPath(const fs::path& path)
```

**Parameters**

- `const fs::path& path`

**Returns** — void

Opens a directory and fires onDirectoryExpanded. Opening one that is already open is a no-op, so the callback fires on the transition rather than on every click.

---

### collapsePath

```cpp
collapsePath(const fs::path& path)
```

**Parameters**

- `const fs::path& path`

**Returns** — void

Closes a directory and fires onDirectoryCollapsed. Expansions nested inside it are deliberately left in the set, so reopening the directory restores the shape the user had before collapsing it.

---

### togglePath

```cpp
togglePath(const fs::path& path)
```

**Parameters**

- `const fs::path& path`

**Returns** — void

Opens a closed directory or closes an open one.

---

### collapseAll

```cpp
collapseAll()
```

**Returns** — void

Closes every directory. The root is deliberately absent from the expanded set -- it is the header, and its children are always the top level of the list -- so this collapses back to that level rather than to nothing.

---

### setSelectedPath

```cpp
setSelectedPath(const fs::path& path)
```

**Parameters**

- `const fs::path& path`

**Returns** — void

Marks an entry as selected, rebuilding so the row picks up the selection colours.

---

### clearSelection

```cpp
clearSelection()
```

**Returns** — void

Drops the selection, so no row is highlighted.

---

### handleEntryClicked

```cpp
handleEntryClicked(const fs::path& path, bool isDirectory)
```

**Parameters**

- `const fs::path& path`
- `bool isDirectory`

**Returns** — void

The one place a click on any entry row lands. Updates the selection when the browser is selectable, then fires the callback for the kind of entry that was hit and expands or collapses a directory if that is configured.

---

### clearRows

```cpp
clearRows()
```

**Returns** — void

Marks every row and its contents for deletion and empties the child list.

---

### rebuildRows

```cpp
rebuildRows()
```

**Returns** — void

Rebuilds the whole list from the tree and the current expansion state. The header goes in first and is marked ignoreScroll, so the scrollable [Column](../containers/Column.hpp.md#column) pins it to the top and scrolls only the entries beneath it. The root itself is never a row: its children are the top level of the list.

---

### rootDisplayName

```cpp
rootDisplayName()
```

**Returns** — std::string

Display name for the root. A relative root has no useful filename ("../" gives ".."), so the path is resolved to an absolute one and normalised first, which makes it read as the folder it actually points at. Normalising leaves a trailing separator on a directory and so an empty filename, in which case the parent's name is used; at a filesystem root even that is empty, and the path itself is shown.

---

### buildHeader

```cpp
buildHeader()
```

**Returns** — [Element](../Element.hpp.md#element)* -- the pinned title strip naming the root

Builds the header row: an optional left inset, the root's icon, a gap, and the root's name. Marked ignoreScroll so the panel pins it above the scrolling list.

---

### appendDirectory

```cpp
appendDirectory(Directory* dir, int depth)
```

**Parameters**

- `Directory* dir`
- `int depth`

**Returns** — void

Appends a row for each of a directory's filtered, sorted entries, recursing into any child directory the user has opened. Listing a directory is what pulls it off disk the first time; afterwards the [FileTree](../../utils/FileTree.hpp.md#filetree) serves it from cache.

---

### passesFilter

```cpp
passesFilter(const FSEntry* entry)
```

**Parameters**

- `const FSEntry* entry`

**Returns** — bool

Whether an entry should be listed at all, testing the directory/file toggles, the hidden-file rule, and the extension filter. Extensions may be given with or without a leading dot and are matched case-insensitively; an empty filter lists everything.

---

### sortEntries

```cpp
sortEntries(std::vector<FSEntry*>& entries)
```

**Parameters**

- `std::vector<FSEntry*>& entries`

**Returns** — void

Orders one directory's entries by the configured mode. Directories sort ahead of files first when directoriesFirst is on, which is what keeps the tree readable; within a group the mode decides, and every mode falls back to the name so the order is stable when two entries tie. Size and modification time read as 0 for a directory, since neither is listed for one.

---

### appendEntryIcon

```cpp
appendEntryIcon(...)
```

**Parameters**

- `std::vector<Element*>& children`
- `const FSEntry* entry`
- `const Color& textColor`
- `const std::string& textRole`
- `bool selected`

**Returns** — void

Appends the icon that marks what kind of entry this is, followed by the gap before the label. An untinted icon rides the entry's own text colour, so selection and theming carry the glyph along without extra configuration; a configured icon colour is dropped on a selected row so the selection reads as one block.

---

### appendDirectoryArrow

```cpp
appendDirectoryArrow(...)
```

**Parameters**

- `std::vector<Element*>& children`
- `const FSEntry* entry`
- `const Color& textColor`
- `const std::string& textRole`
- `bool selected`

**Returns** — void

Appends the arrow that marks whether a directory is open. It goes in after the label, which is the only percent-sized child of the row: the label takes whatever width the fixed children leave, so the arrow ends up pinned to the right edge however wide the row is. A file has nothing to expand and reserves the arrow's width with a [Spacer](../decoration/Spacer.hpp.md#spacer) instead, which keeps the right-hand gutter -- and so the point where a long name clips -- the same on every row. The arrow carries no handlers of its own, so clicking it falls through to the row and toggles the directory.

---

### buildRow

```cpp
buildRow(FSEntry* entry, int depth)
```

**Parameters**

- `FSEntry* entry`
- `int depth`

**Returns** — [Element](../Element.hpp.md#element)* -- the row for one entry

Builds one entry row: indent, icon, gap, label, and the directory arrow. The label is name and extension only -- the full path is never shown. Colours are derived from whether the entry is a directory and whether it is the selected row, and every child before the arrow is decoration with no handlers of its own, so the row's own click always wins. Hover swaps the row background and restores the resolved base on exit; a selected row keeps its highlight because a rebuild re-derives both colours from the current selection.

---

### addEntryRow

```cpp
addEntryRow(Element* row)
```

**Parameters**

- `Element* row`

**Returns** — void

Appends a row, preceded by the entry-spacing gap when one is configured and this is not the first row. The gap is a [Spacer](../decoration/Spacer.hpp.md#spacer) rather than a taller slot, so it stays out of the row's own bounds and is neither clickable nor part of the hover target.

---

### iconNameFor

```cpp
iconNameFor(const FSEntry* entry)
```

**Parameters**

- `const FSEntry* entry`

**Returns** — const std::string&

Which registry icon represents an entry: the folder icon for a directory, or its open variant when one is configured and the directory is expanded; otherwise the [FileKind](../../utils/FileTree.hpp.md#filekind) mapping when kind icons are on, falling back to the plain file icon.
