# SVG & Icons in UILO

How UILO should handle SVG artwork across its two backends (bgfx desktop, Wt
web). This is a design/decision doc, not an implementation.

## TL;DR

- **Add an `Icon` element**, not an SVG renderer. It holds the *source* (SVG
  markup / path data), and each backend draws it its own way.
- **Scope to icons, not full SVG.** Full SVG (filters, masks, patterns, text,
  CSS) is a multi-year vector-graphics engine. UI icons are a tiny subset:
  a few `<path>`s + a solid fill + a `viewBox`.
- **Don't parse SVG yourself** — use **NanoSVG** (two MIT headers).
- **Don't hand-roll the desktop rasterizer/tessellator** — that (anti-aliasing,
  concave tessellation, stroking) is the genuinely hard part libraries solve.
- **Desktop: start by rasterizing to a bgfx texture** (reuses the `Image`
  path, ships fast); **upgrade to [vg-renderer](https://github.com/jdryg/vg-renderer)**
  (vector-on-bgfx) when you want crisp-at-any-scale + trivial theming.
- **Web: emit inline `<svg>`** — the browser renders it for free.
- The `Icon` element API stays identical across all of the above, so raster vs.
  vector stays isolated to the renderer.

## Scope: icons, not full SVG

The SVG format is enormous — paths, shapes, linear/radial gradients, patterns,
strokes (caps/joins/miter/dashes), transforms, clip paths, masks, `<use>`/
`<defs>`, filters (blur/drop-shadow/color-matrix), text (font matching +
layout), CSS, blend modes. A conformant renderer is a 2D vector-graphics engine
and is explicitly **out of scope**.

What UILO needs is what ~99% of UI SVG is: **icons** — one or a few `<path>`s,
a solid (occasionally gradient) fill, a `viewBox`. Material / Lucide / Feather /
Font Awesome are all essentially that. Scope the feature to this subset and the
problem becomes small and ownable.

## The two halves: easy vs. hard

It's tempting to think "an SVG is just instructions for drawing, and I control
the renderer, so I'll interpret them myself." Half of that is true:

- **Easy (own it):** *interpreting* the format — parse path data into
  move/line/cubic/close + fill/stroke color. A weekend, and NanoSVG already
  does it.
- **Hard (don't own it):** the *rendering backend* — turning arbitrary
  filled/stroked Bézier paths into crisp, anti-aliased triangles for the GPU.
  That means path flattening, robust concave-polygon **tessellation**
  (nonzero/even-odd winding, self-intersections), **stroke geometry** (joins,
  caps, dashes), and **anti-aliasing** that doesn't shimmer on thin strokes and
  sharp corners. This is exactly what NanoVG and vg-renderer exist for, and the
  edge cases are where months disappear.

So: own the element and the integration; lean on a library for the
vector-rasterization grind — the same call you already made vendoring bgfx
instead of writing a GPU abstraction.

## Architecture: one `Icon` element, two backends

The `Icon` element is backend-agnostic. It stores the SVG **source** (raw markup
or a file path) plus a resolved fill/tint, and optionally a cached parse. Each
backend consumes that differently — mirroring UILO's existing "one tree, two
backends" model:

```
            Icon element  (holds SVG source + fill/role)
                  |
     ┌────────────┴─────────────┐
  desktop (bgfx)              web (Wt, via toWt)
  parse -> draw/raster        emit inline <svg> (or <img>)
```

Keeping the *source* on the element (not backend-specific pixels or triangles)
is what lets the same `Icon` in the UILO tree render natively on desktop and as
real SVG in the browser.

## Parsing: NanoSVG

Use [NanoSVG](https://github.com/memononen/nanosvg) (`nanosvg.h` +
`nanosvgrast.h`, MIT, header-only). It parses paths, basic shapes, and gradients
into flattened shape data, and ships an optional CPU rasterizer. It's the
de-facto "get the paths out of an SVG" choice for games/UIs.

Vendor it under `ext/` alongside `bx`/`bimg`/`bgfx` and clone it from `build.sh`
the same way (see Integration).

## Desktop backend (bgfx)

Two routes. Both use NanoSVG to parse; they differ in how paths reach the
screen. The `Icon` API is identical for both — pick per phase.

### Option A — rasterize to a texture (start here)

1. Parse with NanoSVG.
2. Rasterize to RGBA at the icon's on-screen pixel size with `nanosvgrast.h`
   (or lunasvg/plutovg if you want better fidelity).
3. Upload as a bgfx texture and draw it with the **existing `Image` quad path**.

- **Pros:** ships in ~a day; dead reliable; cross-platform for free; reuses the
  renderer's texture path.
- **Cons:** it's raster — re-rasterize on size/DPI change to stay crisp (cache
  keyed by `{source, pxSize, color}`); monochrome tinting needs a multiply in
  the shader or a re-raster with the new color.
- **Best when:** icons at known sizes, minimal animation. For a first cut, this
  is the right call.

### Option B — vector on bgfx via vg-renderer (the upgrade)

[vg-renderer](https://github.com/jdryg/vg-renderer) is a vector renderer **built
for bgfx** — a strong fit since UILO already vendors bgfx/bx/bimg. Feed it the
NanoSVG paths (`moveTo`/`cubicTo`/`fill`/`stroke`) and it does tessellation, AA,
and gradients on the GPU.

- **Pros:** true resolution independence (crisp at any scale/DPI), cheap to
  animate/transform, and theming is trivial — set the fill from a palette role,
  no tint shader, no re-raster. This is the right long-term answer for a
  polished, scalable, theme-driven toolkit.
- **Cons:** a vendored dependency and some integration into the frame; the
  project is somewhat niche.
- **Best when:** you want crisp icons at arbitrary DPI, smooth zoom, animated or
  theme-tinted vector art.

NanoVG is the fallback alternative if vg-renderer proves unmaintained; same idea,
slightly different bgfx integration.

### Why not hand-roll it

Writing the tessellation + stroking + AA + gradient pipeline yourself is
reimplementing NanoVG/vg-renderer. Getting robust AA and correct concave/
self-intersecting fills is genuinely hard and full of edge cases. Only justified
as a deliberate learning project or if you need something the libraries can't
express — not for shipping a UI toolkit.

## Web backend (Wt)

In the `toWt` translator, an `Icon` becomes **inline `<svg>`** (preferred — the
markup styles via CSS, so `fill` follows the theme) or a `<img src="data:...">`
for a file. The browser renders SVG natively: crisp, resolution-independent, and
free. Recolor by setting `fill` / `color` from the same palette role the desktop
side uses.

This is strictly easier than desktop — no rasterization at all — so the desktop
path is the one that drives the design.

## Theming / tinting

Icons should follow UILO's palette like everything else, via `setColorRole`:

- **Vector (Option B) / web:** set the path fill color directly from the
  resolved role — instant, crisp, free.
- **Raster (Option A):** the rasterized texture is fixed-color; tint with a
  multiply in the shader, or re-rasterize when the role's color changes
  (palette switch). Cache accordingly.

Monochrome icons (the common case) tint cleanly; multicolor art keeps its own
fills unless a tint is forced.

## The `Icon` element (API sketch)

Consistent with UILO's `Modifier` + `Options` + factory pattern:

```cpp
// Factory (Factory.hpp), alongside image()/text():
Icon* icon(Modifier modifier = {}, IconOptions options = {},
           const std::string& name = "");

class IconOptions {
public:
    IconOptions& setMarkup(const std::string& svg);   // raw <svg>... string
    IconOptions& setFile(const std::string& path);     // or a .svg on disk
    IconOptions& setColor(const Color& c);             // fill / tint override
    IconOptions& setColorRole(const std::string& r);   // palette-driven fill
    IconOptions& setPreserveAspect(bool v = true);     // fit vs. stretch to bounds
    // getters ...
private:
    std::string m_markup, m_file, m_colorRole;
    Color       m_color = Color::White;
    bool        m_preserveAspect = true;
};

class Icon : public Element {
    // Holds IconOptions + a cached parse (NanoSVG image) on desktop; the raw
    // markup is what toWt emits on web. Size comes from the Modifier (width/
    // height, _px/_pct), fill from color/colorRole resolved through the Palette.
};
```

Notes:
- Size is driven by the `Modifier` (`setWidth`/`setHeight`), like every element.
- The element stores *source*; the desktop parse/raster/tessellation is a
  renderer concern and cached, not part of the element's public surface.

## Integration points (files to touch)

- `include/elements/Element.hpp` — add `ElementType::Icon`.
- `include/elements/decoration/Icon.hpp` / `Icon.cpp` — the element (sibling to
  `Image`).
- `include/elements/Factory.hpp` — the `icon(...)` factory.
- `include/renderer/Renderer.*` — desktop draw: Option A adds an SVG→texture
  upload (reusing the image draw); Option B adds vg-renderer path submission.
- `include/wt/Translator.cpp` — add the `Icon` case → inline `<svg>` / `<img>`,
  fill from the palette role (this is where the earlier scrollbar/CSS helpers
  live).
- `ext/` — vendor `nanosvg` (and, for Option B, `vg-renderer`).
- `build.sh` — `clone_if_missing nanosvg …` (and `vg-renderer`), same pattern as
  `bx`/`bimg`/`bgfx`; nothing to build for NanoSVG (header-only).
- `CMakeLists.txt` — add the include dir(s); link vg-renderer for Option B.

## Roadmap

- **Phase 0 — element + parse.** `Icon`/`IconOptions`/`icon()`, vendor NanoSVG,
  parse markup/file into cached shape data. No drawing yet.
- **Phase 1 — web.** `toWt` emits inline `<svg>` with palette-driven `fill`.
  (Easiest backend; validates the element end-to-end in the web app first.)
- **Phase 2 — desktop raster.** NanoSVG rasterize → bgfx texture → existing
  image quad, cached by `{source, size, color}`. Icons now work on desktop.
- **Phase 3 — desktop vector.** Swap the desktop backend to vg-renderer for
  resolution independence + direct fill theming. `Icon` API unchanged.

## Non-goals (for now)

Filters (blur, drop-shadow, color-matrix), masks, clip paths, patterns, SVG
`<text>`, CSS styling, animation (SMIL/CSS). If a specific icon needs one of
these, prefer pre-baking it (e.g. flatten in a design tool) over growing the
renderer. Full-SVG-document support is a separate, much larger project.

## Open questions

- **Source form:** inline markup vs. file path vs. a compiled icon registry
  (name → SVG). A registry keeps call sites clean (`icon(..., "chevron")`) and
  lets web and desktop share one source; probably worth it once there are more
  than a handful.
- **Raster vs. vector timeline:** ship Phase 2 (raster) and stop if icons are
  always fixed-size, or push to Phase 3 (vg-renderer) if crisp-at-scale /
  animated / heavily themed icons matter. UILO's `setScale`/DPI story argues for
  eventually landing on vector.
- **Caching ownership:** where the desktop parse/raster cache lives (per-`Icon`
  vs. a renderer-side cache keyed by content) and when it's invalidated
  (size/DPI/palette change).

## References

- NanoSVG — <https://github.com/memononen/nanosvg>
- vg-renderer — <https://github.com/jdryg/vg-renderer>
- NanoVG — <https://github.com/memononen/nanovg>
- lunasvg / plutovg (higher-fidelity CPU rasterizers) —
  <https://github.com/sammycage/lunasvg>
