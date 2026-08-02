# Container.hpp

`include/elements/containers/Container.hpp`

[← index](../../README.md)

## Types

- [Container](#container)

---

### Container

Base class for every element that holds others. It owns the child list and the pointer-event plumbing -- clicks, hover, scroll and zoom are offered to the children before the container itself gets a look -- and leaves layout and drawing abstract for [Row](Row.hpp.md#row), [Column](Column.hpp.md#column) and [Canvas](Canvas.hpp.md#canvas) to define. Children are raw pointers, since [UILO](../../UILO.hpp.md#uilo)'s element pool owns them; pruneChildren() drops any that have been erased, and m_fb is the container's own render target.
