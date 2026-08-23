# Icon.hpp

`include/elements/decoration/Icon.hpp`

[← index](../../README.md)

## Types

- [IconOptions](#iconoptions)
- [Icon](#icon)

## Functions

- [`getStrokeWidth()`](#getstrokewidth)
- [`hasStrokeWidth()`](#hasstrokewidth)

---

### IconOptions

Styling for an [Icon](#icon). The source is one of three things, checked in the order markup, then file, then registry name: setIcon("arrow-left")          -- a name in [Resources](../../utils/Resources.hpp.md#resources) setIcon([Resources](../../utils/Resources.hpp.md#resources)::icons::x)   -- the same, spelled safely setFile("art/logo.svg")        -- an .svg on disk setMarkup("<svg>...</svg>")    -- markup already in memory The built-in set is monochrome line art authored with stroke="currentColor", which no SVG parser can resolve on its own, so the colour set here is applied to every stroke and fill in the icon. That is what makes tinting an icon a one-liner -- IconOptions().setColorRole("accent") -- and setPreserveOriginalColors(true) opts out, keeping whatever colours the markup declares, for multicolour art.

> Stroke width is in the icon's own authoring units, the numbers in the markup, not pixels. The built-ins are drawn on a 24x24 grid at width 1.5, so setStrokeWidth(2.f) is "a bit heavier than stock" at every size, and the stroke keeps its proportions as the icon scales.

---

### Icon

Draws an SVG icon. The element holds the *source* markup: the desktop backend parses it with NanoSVG and rasterizes to a texture at the size the icon actually occupies on screen, so it stays crisp as the window scales, while the web backend can emit that same markup inline instead. Keeping source rather than pixels on the element is what lets one Icon serve both.

> The raster is cached and rebuilt only when something it depends on has moved -- pixel size, resolved colour, stroke width, or the source itself. A palette switch therefore re-tints without the caller doing anything.

---

### getStrokeWidth

```cpp
getStrokeWidth()
```

**Returns** — float

Stroke width in the icon's own authoring units. Only meaningful when hasStrokeWidth() is true, since 0 is also what an unset width reports.

---

### hasStrokeWidth

```cpp
hasStrokeWidth()
```

**Returns** — bool

Whether a stroke width was set, here or by the theme this icon was built from. Only then is the width the markup declares overridden, so a themed stroke reaches every icon while an unthemed one keeps whatever its art was authored with.
