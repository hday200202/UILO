# Page.hpp

`include/Page.hpp`

[← index](README.md)

## Types

- [Page](#page)

---

### Page

A named screen owned by [UILO](UILO.hpp.md#uilo), wrapping a root [Container](elements/containers/Container.hpp.md#container). [UILO](UILO.hpp.md#uilo) ticks and renders the active page each frame, and the page forwards layout and draw calls to its root container. Members are protected and driven through [UILO](UILO.hpp.md#uilo) via friendship.
