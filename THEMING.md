# UILO Theming — Design Notes

Brainstorming for a built-in theming system. Not yet implemented.

## Goal

Let consumers change colors (and eventually fonts/metrics) at runtime
without:

- Holding a reference to every element they want to recolor.
- Walking and downcasting the element tree.
- Rebuilding pages (which destroys element state — scroll positions,
  textbox contents, resizer-adjusted sizes, etc.).

Themes should be loadable from disk (JSON), hot-swappable, and not
force every new widget author to extend a central enum.

## Considered approaches

### 1. Color-by-reference

```cpp
.setColor(&theme.panelColor) // element stores Color*
```

- **Pros:** zero plumbing, trivial call site.
- **Cons:** lifetime hazards (stack-local themes, copies), forces
  every `Options` struct to accept both `Color` and `Color*`.
  Doesn't compose (no "panel darkened by 10%").

Rejected — too easy to misuse.

### 2. Per-MULO traversal with cached (element, role) pairs

A `track(element, role)` helper on the *consumer* side registers
elements as they're built; `applyTheme()` loops the list.

- **Pros:** no UILO changes needed.
- **Cons:** consumer-side bookkeeping; every dynamically-created
  element must register; rebuild requires clearing the list.

Workable as an interim, but doesn't belong in user code long-term.

### 3. Palette class inside UILO (current favorite)

A `Palette` owned by `UILO` that maps role names to colors. Elements
store a role identifier instead of (or in addition to) a literal
`Color`. Render path resolves role -> color each frame.

```cpp
class Palette {
public:
    void  set(const std::string& role, Color c);
    Color get(const std::string& role) const;
    bool  has(const std::string& role) const;
    void  setFallback(Color c);

    bool loadJson(const std::string& path);
    bool reload();
};
```

Element API:

```cpp
options.setColor(Color::Red);          // literal, existing behavior
options.setColorRole("panel");         // resolved per-frame via Palette
```

Resolution: `role.empty() ? literal : palette.get(role)`.

Changing the theme = `palette.set("panel", newColor)` (or
`palette.loadJson("dark.json")`). Every element that uses that role
updates next frame. No traversal, no rebuild, no cached pointers, and
dynamically-created elements participate for free.

#### Storage / perf

- Start with `unordered_map<string, Color>`. Per-frame hash is fine.
- If profiling complains: add `RoleId Palette::intern(string)` that
  returns a small int; elements store the int. Lookup becomes
  vector index. String-facing API stays the same.

#### Where Palette lives

Per-`UILO` instance, reached through the existing element ->
`UILO*` back-pointer. Multi-window-safe; no globals.

## The "every element has its own colors" problem

Sliders need track / fill / thumb / thumb-hover. Buttons need bg /
bg-hover / text. Knobs need ring / arc / indicator. A flat palette
key per color does not scale if **UILO** has to enumerate every role.

### Resolution: namespaced keys, widget-defined

Keep `Palette` flat (`map<string, Color>`). Each widget author picks
the keys *their* widget reads, with sensible defaults:

```cpp
SliderOptions()
    .setTrackRole("slider.track")
    .setFillRole("slider.fill")
    .setThumbRole("slider.thumb");
```

UILO core never grows a registry. New widgets — including ones a
consumer writes outside UILO — add their own namespaced keys without
touching shared code. The theme file just lists more entries.

```json
{
  "bg":            "#21232f",
  "panel":         "#2c2f3c",
  "accent":        "#6aa3ff",

  "slider.track":  "$panel",
  "slider.fill":   "$accent",
  "slider.thumb":  "#ffffff",

  "button.bg":     "$panel",
  "button.bgHover":"$accent",
  "button.text":   "#ffffff"
}
```

`$role` syntax = alias to another role. Resolved on load or lazily on
`get`. Lets a theme declare a small base palette and point every
widget-specific role at it; swap one base color, dozens of widgets
update.

### Tradeoff acknowledged

This pushes responsibility onto the widget author: every new widget
means new role keys. Mitigations:

- Provide a `Palette::defaultDark()` / `defaultLight()` that
  pre-populates the common keys UILO's built-in widgets use, so
  consumers with no JSON still get sensible colors.
- A debug helper that warns the first time a role resolves to the
  fallback color (catches typos in JSON or missing keys).

### Alternative: structured per-widget styles

```json
{
  "colors": { "panel": "#2c2f3c", "accent": "#6aa3ff" },
  "slider": { "track": "$panel", "fill": "$accent", "thumb": "#fff" }
}
```

Palette returns `SliderStyle` structs. Cleaner data model, but UILO
needs a registry of style types per element. Locks third-party widget
authors out of theming without recompiling UILO. **Rejected** in
favor of flat namespaced keys.

## JSON loader

- Color formats: `"#rrggbb"`, `"#rrggbbaa"`, `"rgb(r,g,b)"`,
  `"rgba(r,g,b,a)"`, `"$other.role"`.
- `Palette::loadJson(path)` merges into the current palette
  (doesn't wipe), so user themes can layer on top of defaults.
- `Palette::reload()` re-reads the last file — free hot reload as
  long as elements re-resolve each frame.
- JSON dep: pick between `nlohmann::json` (one header, ~30k LoC, easy
  to use, big dep) and a tiny hand-rolled parser (this is leaf data,
  no nesting beyond depth-2, totally doable in ~150 LoC).

## Backwards compatibility

`setColor(Color)` keeps working unchanged. Role-based theming is
opt-in per element. Existing code (UILO examples, current MULO usage)
compiles untouched.

## Out of scope for v1

- Derived colors (`"panel darkened 10%"`). Workaround: declare the
  variant as its own role in the theme file.
- Fonts and metrics. Same pattern works; defer until colors land.
- Animated theme transitions.

## Suggested implementation order

1. `Palette` class + literal-color fallback path.
2. `setColorRole` on `Modifier` (or wherever color lives) for the
   primary container/decoration elements (`Row`, `Column`, `Text`).
3. Per-widget role setters on interactibles as they're touched.
4. JSON loader.
5. Defaults (`defaultDark` / `defaultLight`).
6. Debug "unresolved role" warning.
