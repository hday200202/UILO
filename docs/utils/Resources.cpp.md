# Resources.cpp

`include/utils/Resources.cpp`

[← index](../README.md)

## Functions

- [`get()`](#get)
- [`findBuiltIn(std::string_view name)`](#findbuiltin)
- [`find(std::string_view name)`](#find)
- [`add(std::string name, std::string markup)`](#add)
- [`addFile(std::string name, const std::filesystem::path& file)`](#addfile)
- [`addDirectory(const std::filesystem::path& dir)`](#adddirectory)
- [`remove(std::string_view name)`](#remove)
- [`names()`](#names)
- [`size()`](#size)
- [`builtInCount()`](#builtincount)
- [`FontRegistry()`](#fontregistry)
- [`resolve(std::string_view nameOrPath)`](#resolve)
- [`add(std::string name, std::string path)`](#add)
- [`remove(std::string_view name)`](#remove)
- [`contains(std::string_view name)`](#contains)
- [`names()`](#names)

---

### get

```cpp
get()
```

**Returns** — [Resources](Resources.hpp.md#resources)&

The process-wide registries. A single instance so an icon or font registered anywhere is visible everywhere, including to elements built before the application set them up.

---

### findBuiltIn

```cpp
findBuiltIn(std::string_view name)
```

**Parameters**

- `std::string_view name`

**Returns** — std::string_view

The generated table is sorted by name, so a built-in lookup is a binary search over string_views with no allocation.

---

### find

```cpp
find(std::string_view name)
```

**Parameters**

- `std::string_view name`

**Returns** — std::string_view [Resources](Resources.hpp.md#resources)::

The SVG markup registered under a name, empty when the name is unknown. Application-registered icons are searched before the built-in set, so a project can override a built-in by reusing its name.

---

### add

```cpp
add(std::string name, std::string markup)
```

**Parameters**

- `std::string name`
- `std::string markup`

**Returns** — void [Resources](Resources.hpp.md#resources)::

Registers SVG markup under a name, replacing any earlier entry.

---

### addFile

```cpp
addFile(std::string name, const std::filesystem::path& file)
```

**Parameters**

- `std::string name`
- `const std::filesystem::path& file`

**Returns** — bool [Resources](Resources.hpp.md#resources)::

Registers an icon from an .svg on disk, read once at call time so the file need not outlive the call. Reports false when it cannot be read.

---

### addDirectory

```cpp
addDirectory(const std::filesystem::path& dir)
```

**Parameters**

- `const std::filesystem::path& dir`

**Returns** — std::size_t [Resources](Resources.hpp.md#resources)::

Registers every .svg in a directory, each under its filename without the extension. Returns how many were added, so a caller can tell an empty directory from a missing one.

---

### remove

```cpp
remove(std::string_view name)
```

**Parameters**

- `std::string_view name`

**Returns** — void [Resources](Resources.hpp.md#resources)::

Removes an application-registered icon. The built-in set is not affected, so a name that was overriding a built-in falls back to it.

---

### names

```cpp
names()
```

**Returns** — std::vector&lt;std::string_view&gt; [Resources](Resources.hpp.md#resources)::

A set rather than a plain append: a registered override shares its name with the built-in it shadows and must appear once.

---

### size

```cpp
size()
```

**Returns** — std::size_t [Resources](Resources.hpp.md#resources)::

How many icons are registered in total, built-ins included.

---

### builtInCount

```cpp
builtInCount()
```

**Returns** — std::size_t [Resources](Resources.hpp.md#resources)::

How many icons ship with [UILO](../UILO.hpp.md#uilo), for telling built-ins from additions.

---

### FontRegistry

```cpp
FontRegistry()
```

An empty path is the renderer's existing signal for "use the embedded face", so the default font is a name pointing at nothing rather than a copy of the bytes.

---

### resolve

```cpp
resolve(std::string_view nameOrPath)
```

**Parameters**

- `std::string_view nameOrPath`

**Returns** — std::string_view [Resources](Resources.hpp.md#resources)::

Turns a registered font name into its path. Anything not registered is handed back unchanged, so a plain path passes straight through and callers never have to know which they were given.

---

### add

```cpp
add(std::string name, std::string path)
```

**Parameters**

- `std::string name`
- `std::string path`

**Returns** — void [Resources](Resources.hpp.md#resources)::

Registers a font path under a short name.

---

### remove

```cpp
remove(std::string_view name)
```

**Parameters**

- `std::string_view name`

**Returns** — void [Resources](Resources.hpp.md#resources)::

Unregisters a font name.

---

### contains

```cpp
contains(std::string_view name)
```

**Parameters**

- `std::string_view name`

**Returns** — bool [Resources](Resources.hpp.md#resources)::

Whether a name is registered.

---

### names

```cpp
names()
```

**Returns** — std::vector&lt;std::string_view&gt; [Resources](Resources.hpp.md#resources)::

Every registered font name, for building a font picker.
