# UiloWt.cpp

`include/wt/UiloWt.cpp`

[← index](../README.md)

## Types

- [UiloApplication](#uiloapplication)

## Functions

- [`serverArgs(const WebConfig& config)`](#serverargs)
- [`argPointers(std::vector&lt;std::string&gt;& args)`](#argpointers)
- [`UILO::runWeb(const WebConfig& config)`](#uilo-runweb)
- [`UILO::runWeb(factory, const WebConfig& config)`](#uilo-runweb)

---

### UiloApplication

One browser connection. Renders a [UILO](../UILO.hpp.md#uilo) into this connection's Wt widgets and keeps them in step through a [Translator](Translator.hpp.md#translator). Which [UILO](../UILO.hpp.md#uilo) depends on how the server was started. The shared form takes one by reference and every connection reflects it. The per-session form hands over a [WebApp](../UILO.hpp.md#webapp) this application then owns, so the [UILO](../UILO.hpp.md#uilo) -- and the application state behind it -- is this connection's alone and dies with it.

---

### serverArgs

```cpp
serverArgs(const WebConfig& config)
```

**Parameters**

- `const WebConfig& config`

**Returns** — std::vector&lt;std::string&gt;

Wt is configured from a command line; synthesize one from the config so the caller passes nothing. Returned as strings -- WRun wants char*, which the callers point at these.

---

### argPointers

```cpp
argPointers(std::vector<std::string>& args)
```

**Parameters**

- `std::vector<std::string>& args`

**Returns** — std::vector&lt;char*&gt;

The char* view of serverArgs()'s strings that WRun takes.

---

### UILO::runWeb

```cpp
UILO::runWeb(const WebConfig& config)
```

**Parameters**

- `const wt::WebConfig& config`

**Returns** — int

Serves this [UILO](../UILO.hpp.md#uilo) over the web and blocks until the server stops. Set the [UILO](../UILO.hpp.md#uilo) up exactly as on desktop first (addPage/setPage/setTheme); this stands up Wt, points every browser connection at this instance, and returns the server's exit code. No render/update loop and no argv -- the server settings come from the config. Every connection shares this instance, and Wt drives connections from different threads: two browsers are two views of one UI, with no lock between them. Use the factory overload for anything multi-user.

---

### UILO::runWeb

```cpp
UILO::runWeb(factory, const WebConfig& config)
```

**Parameters**

- `std::function<std::unique_ptr<wt::WebApp>()> factory`
- `const wt::WebConfig& config`

**Returns** — int

The per-session counterpart: serves an app whose [UILO](../UILO.hpp.md#uilo) is built fresh for each browser connection, and blocks until the server stops. `factory` runs once per connection, on that connection's own thread, and builds that session's [UILO](../UILO.hpp.md#uilo) the same way a desktop app builds its one (addPage/setPage/ setTheme). The [WebApp](../UILO.hpp.md#webapp) it returns is owned by the connection and destroyed with it, so sessions share no pages, no palette and no application state -- and cannot race each other's widget trees. Anything the factory itself touches is still shared, so whatever it reads (a global theme, a file on disk) has to be safe to read from several threads at once.
