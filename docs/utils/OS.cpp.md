# OS.cpp

`include/utils/OS.cpp`

[← index](../README.md)

## Functions

- [`videoReady()`](#videoready)
- [`scale()`](#scale)
- [`displaySize()`](#displaysize)
- [`physicalDisplaySize()`](#physicaldisplaysize)
- [`pixelDensity()`](#pixeldensity)
- [`refreshRate()`](#refreshrate)
- [`displayCount()`](#displaycount)
- [`theme()`](#theme)
- [`cpuCount()`](#cpucount)
- [`systemRamMB()`](#systemrammb)
- [`executableDirectory()`](#executabledirectory)
- [`preferencesDirectory(...)`](#preferencesdirectory)

---

### videoReady

```cpp
videoReady()
```

**Returns** — bool

Every display query needs the video subsystem. [UILO](../UILO.hpp.md#uilo)'s renderer initialises it, but OS is callable before that (and from tools that never open a window), so each query checks rather than returning nonsense.

---

### scale

```cpp
scale()
```

**Returns** — float

[UILO](../UILO.hpp.md#uilo) lays out in virtual pixels that map 1:1 onto the window's pixels, so the useful scale is "how many real panel pixels does one virtual pixel cover" -- native panel resolution divided by the virtual desktop size. This is NOT SDL_GetDisplayContentScale, which reports 1.0 on macOS however the desktop is scaled, and NOT the backing-store ratio either: at a 1470x956 desktop macOS renders 2940x1912 and scans it out to a 2560x1664 panel, so the backing ratio says 2.0 while the panel only gives 1.74 physical pixels per point. Using 2.0 would draw everything ~15% too large.

---

### displaySize

```cpp
displaySize()
```

**Returns** — [Vec2u](Math.hpp.md#vec2u)

Prefer the platform's own answer; SDL's bounds agree with it on macOS but this keeps the two halves of the scale ratio coming from one source.

---

### physicalDisplaySize

```cpp
physicalDisplaySize()
```

**Returns** — [Vec2u](Math.hpp.md#vec2u)

The primary display's real panel resolution in pixels. macOS is asked directly, since its framebuffer is not the panel; everywhere else the framebuffer is the panel, so the virtual size times the reported pixel density is the best answer available.

---

### pixelDensity

```cpp
pixelDensity()
```

**Returns** — float

Backing pixels per virtual pixel on the primary display. Falls back to 1 whenever the video subsystem is not up or the display cannot be queried, so a caller never has to guard against 0.

---

### refreshRate

```cpp
refreshRate()
```

**Returns** — float

Refresh rate of the primary display in Hz, 0 when it cannot be determined.

---

### displayCount

```cpp
displayCount()
```

**Returns** — int

How many displays are attached, 0 before the video subsystem is up.

---

### theme

```cpp
theme()
```

**Returns** — OSTheme

Whether the desktop is set to a light or dark appearance, so an application can pick a matching palette without asking the user.

---

### cpuCount

```cpp
cpuCount()
```

**Returns** — int

Number of logical CPUs, for sizing a worker pool.

---

### systemRamMB

```cpp
systemRamMB()
```

**Returns** — int

Installed system memory in megabytes.

---

### executableDirectory

```cpp
executableDirectory()
```

**Returns** — std::string

Owned by SDL and valid for the process lifetime -- do not free.

---

### preferencesDirectory

```cpp
preferencesDirectory(...)
```

**Parameters**

- `const std::string& organization`
- `const std::string& application`

**Returns** — std::string

The per-user directory an application should keep its settings in, created if it does not exist. The organisation and application names become the path components the platform expects, so the result is the conventional location on each one.
