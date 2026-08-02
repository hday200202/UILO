# FileTree.hpp

`include/utils/FileTree.hpp`

[← index](../README.md)

## Types

- [FileKind](#filekind)
- [EntryType](#entrytype)
- [FSEntry](#fsentry)
- [File](#file)
- [Directory](#directory)
- [FileTree](#filetree)

---

### FileKind

Enum for abstract file types. - Scoped (enum class) deliberately: unscoped enumerators named [Image](../elements/decoration/Image.hpp.md#image) and [Text](../elements/decoration/Text.hpp.md#text) would collide with the uilo::[Image](../elements/decoration/Image.hpp.md#image) and uilo::[Text](../elements/decoration/Text.hpp.md#text) element classes.

---

### EntryType

Discriminates the two concrete [FSEntry](#fsentry) subclasses without RTTI/dynamic_cast

---

### FSEntry

Common base for [File](#file) and [Directory](#directory). - Holds everything that's meaningful for both: path, parent, name, permissions, and symlink target. All are captured once via error_code overloads, so a vanished file or broken symlink mid-scan never throws. - type()/isFile()/isDirectory() let callers holding an FSEntry* figure out which concrete type they have and cast accordingly (static_cast is safe once type() has been checked).

---

### File

Abstract representation of a file on disk. - Constructor: File(std::filesystem::path, [Directory](#directory)* parent = nullptr) - getFileName()      -> string name of file without extension - getFileExt()       -> string file extension - getFileKind() -> abstract [FileKind](#filekind) derived from extension (no disk access) - getFileSize()      -> file size in bytes, empty on stat failure - getLastModified()  -> last write time as time_t, empty on stat failure - getPermissions()   -> std::filesystem::perms captured at construction (inherited) - isReadOnly()       -> true if no write permission bit is set (inherited) - getSymlinkTarget() -> resolved target path, if this file is a symlink (inherited) - getPath() -> full path on disk (inherited)

---

### Directory

Abstract representation of a directory on disk. - Constructor: Directory(std::filesystem::path, Directory* parent = nullptr) - expand()     -> lazily lists immediate children (files + subdirectories), caching them in m_children. A no-op if already loaded, so it's safe to call every time a widget "opens" this directory without re-hitting the disk. - invalidate() -> drops the cache; pass reload=true to re-list immediately. - getChildren()/getFiles()/getSubdirectories() -> cached results. - Children are stored as a single vector of [FSEntry](#fsentry) so callers can walk one ordered list and cast per-entry (isDirectory()/isFile()) rather than juggling two parallel containers. - Note: expand() only ever lists one level. Nothing here recurses automatically, so a symlink that loops back to an ancestor directory is harmless unless something walks the tree eagerly.

---

### FileTree

Builds and owns a lazily-expanded directory tree rooted at a given path. - Constructor: FileTree(std::filesystem::path rootPath) Expands the root immediately so the top level is ready to display; everything below the root stays unloaded until a widget calls expand() on the [Directory](#directory) it's opening. - getRoot() -> root [Directory](#directory)* - refresh()  -> drops and re-lists the root's immediate children
