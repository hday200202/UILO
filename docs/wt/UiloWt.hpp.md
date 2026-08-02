# UiloWt.hpp

`include/wt/UiloWt.hpp`

[← index](../README.md)

## Types

- [Config](#config)
- [Session](#session)

## Functions

- [`run(int argc, char** argv, Builder build, Config config)`](#run)

---

### Config

Per-application settings that have no [UILO](../UILO.hpp.md#uilo) equivalent, because they describe the page rather than the UI inside it.

---

### Session

One browser tab. Wt builds an application instance per session, so each gets its own [UILO](../UILO.hpp.md#uilo) instance and its own element tree -- element pointers captured in your callbacks belong to that session alone and are never shared between users.

---

### run

```cpp
run(int argc, char** argv, Builder build, Config config)
```

**Parameters**

- `int argc`
- `char** argv`
- `Builder build`
- `Config config`

**Returns** — int -- the server's exit code

Starts the web server and serves `build` at the document root. Blocks until the server stops, so it is a drop-in for the body of main(). Argv is Wt's own -- `--http-address`, `--http-port`, `--docroot`.
