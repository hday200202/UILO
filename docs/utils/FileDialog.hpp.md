# FileDialog.hpp

`include/utils/FileDialog.hpp`

[← index](../README.md)

## Types

- [FileFilter](#filefilter)
- [FileDialogOptions](#filedialogoptions)

## Functions

- [`fileDialogsAvailable()`](#filedialogsavailable)

---

### FileFilter

One entry in a dialog's type menu: a label, and the extensions it matches. Extensions are given bare -- "png", not "*.png" -- and each platform is handed the spelling it expects.

---

### FileDialogOptions

What to put on the dialog before it opens. Every field is optional; a default-constructed one gives the platform's own defaults.

---

### fileDialogsAvailable

```cpp
fileDialogsAvailable()
```

**Returns** — bool

Whether a dialog can actually be shown. True on macOS and Windows; on Linux it depends on a portal or zenity/kdialog being installed, so a headless or minimal system reports false and the four calls above all return nothing rather than hanging.
