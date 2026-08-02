# Resizer.hpp

`include/elements/interactible/Resizer.hpp`

[← index](../../README.md)

## Types

- [ResizerDir](#resizerdir)
- [ResizerOptions](#resizeroptions)
- [Resizer](#resizer)

## Functions

- [`setTarget(Element* t)`](#settarget)

---

### ResizerDir

Which neighbour a [Resizer](#resizer) drags, and so which axis it works on. Left and Top resize the previous sibling, Right and Bottom the next one; Left/Right drag horizontally, Top/Bottom vertically.

---

### ResizerOptions

Which neighbour the handle resizes, how wide its hit strip is, what it draws as, and the limits the drag is held within. The min, max and step limits are Dimensions, so they can be given in pixels or as a percent of the container -- a percent minimum keeps a panel usable at any window size. A step above 0 quantises the drag to that increment.

> The fill is transparent by default, so a resizer stays invisible until a hover handler colours it in.

---

### Resizer

A drag handle placed inside a [Row](../containers/Row.hpp.md#row) or [Column](../containers/Column.hpp.md#column) beside the element it resizes. It occupies a strip for hit detection but is invisible to layout -- its siblings are placed as though it were not there, and it sits at the boundary between them -- so adding one never shifts the arrangement. It draws through [UILO](../../UILO.hpp.md#uilo)'s post- render pass, which puts it on top of everything including the neighbours it straddles.

> The target and the container bounds are pushed in by [Row](../containers/Row.hpp.md#row) and [Column](../containers/Column.hpp.md#column) during layout rather than looked up here, because only the parent knows which sibling is adjacent once invisible children have been skipped.

> Double-clicking restores the target to the size it had when the handle first attached, which is why the original dimension is captured on the first setTarget rather than at construction -- there is no target yet then.

---

### setTarget

```cpp
setTarget(Element* t)
```

**Parameters**

- `Element* t`

**Returns** — void

Attaches the element this handle drags. The target's declared size is captured the first time a real one arrives, so a double- click can restore it later; the capture is guarded because layout calls this every frame, and re-reading the size after a drag would make "restore" mean "whatever it was last frame".
