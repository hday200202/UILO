# MacStubs.cpp

`include/platform/MacStubs.cpp`

[← index](../README.md)

## Functions

- [`configureMacWindowForLiveResize(void*)`](#configuremacwindowforliveresize)
- [`getNativeDisplayPixelSize(uint32_t&, uint32_t&)`](#getnativedisplaypixelsize)
- [`getVirtualDisplaySize(uint32_t&, uint32_t&)`](#getvirtualdisplaysize)
- [`installMacScrollMonitor(std::function&lt;bool(float, float, bool)&gt;)`](#installmacscrollmonitor)
- [`installMacZoomMonitor(std::function&lt;bool(float)&gt;)`](#installmaczoommonitor)
- [`tickMacScrollMomentum(float)`](#tickmacscrollmomentum)
- [`cancelMacScrollMomentum()`](#cancelmacscrollmomentum)

---

### configureMacWindowForLiveResize

```cpp
configureMacWindowForLiveResize(void*)
```

**Parameters**

- `void*`

**Returns** — bool

No CoreGraphics mode list here either, so OS falls back to the SDL-derived size.

---

### getNativeDisplayPixelSize

```cpp
getNativeDisplayPixelSize(uint32_t&, uint32_t&)
```

**Parameters**

- `uint32_t&`
- `uint32_t&`

**Returns** — bool

No equivalent of the CoreGraphics mode list here; OS falls back to the SDL-derived sizes, which is the right answer on platforms that do not render at one resolution and scan out at another.

---

### getVirtualDisplaySize

```cpp
getVirtualDisplaySize(uint32_t&, uint32_t&)
```

**Parameters**

- `uint32_t&`
- `uint32_t&`

**Returns** — bool

No AppKit event monitor to install; SDL's own wheel handling is used unchanged, which is adequate everywhere except macOS.

---

### installMacScrollMonitor

```cpp
installMacScrollMonitor(std::function<bool(float, float, bool)>)
```

**Parameters**

- `std::function<bool(float`
- `float`
- `bool)>`

**Returns** — bool

No AppKit magnification gesture to monitor; pinch zoom arrives, if at all, through SDL.

---

### installMacZoomMonitor

```cpp
installMacZoomMonitor(std::function<bool(float)>)
```

**Parameters**

- `std::function<bool(float)>`

**Returns** — bool

Nothing to tick: momentum is synthesised only on macOS, where SDL drops the phase events AppKit sends.

---

### tickMacScrollMomentum

```cpp
tickMacScrollMomentum(float)
```

**Parameters**

- `float`

Nothing to cancel, for the same reason.

---

### cancelMacScrollMomentum

```cpp
cancelMacScrollMomentum()
```

Nothing to cancel, since no momentum is ever synthesised here.
