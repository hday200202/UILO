#pragma once

#include "Color.hpp"

#include <string>
#include <utility>

namespace uilo {

class Palette;   /* resolve() walks it at draw time — see Gradient.cpp */


/*
    GradientColor:
    - Desc:     One corner of a gradient: a literal Color, or a palette role
                resolved at draw time. Converts implicitly from Color and from a
                role string, so call sites just pass a color or a role name.
*/
struct GradientColor {
    Color       color { 0, 0, 0, 0 };
    std::string role;   /* non-empty -> resolved via palette */

    GradientColor() = default;
    GradientColor(Color c) : color(c) {}
    GradientColor(const char* paletteRole) : role(paletteRole) {}
    GradientColor(std::string paletteRole) : role(std::move(paletteRole)) {}

    bool operator==(const GradientColor& o) const { return color == o.color && role == o.role; }
    bool operator!=(const GradientColor& o) const { return !(*this == o); }
};

/* Deprecated alias for the old name. Prefer GradientColor. */
using GradientStop = GradientColor;

/*
    Gradient:
    - Desc:     A per-corner background fill, built once and shared across
                elements the same way a Material is. Each of the four corners is
                a GradientColor -- a literal Color or a palette role, freely
                mixable, and since GradientColor converts implicitly from both
                you never name the type at a call site. Colors are interpolated
                across the background quad on the GPU and clipped by the same
                rounded-corner mask a solid fill uses, so gradients and rounding
                compose at no extra cost.
    - The fluent setters name the position each colour occupies, so a gradient
      reads without having to remember corner order:

          RowOptions().setGradient({tl, tr, bl, br});
          RowOptions().setGradient({"accent", "accent", "panel", "panel"});
          Gradient().setTop(Color{60, 40, 120}).setBottom(Color{20, 20, 40});
          Gradient().setLeft("accent").setRight("panel");

    - vertical() and horizontal() are shorthand for the two common cases. A
      whole gradient can also be named in the Palette with setGradient("hero",
      g) and referenced per element with setGradientRole("hero"), which is what
      lets a palette switch restyle every gradient at once.
*/
struct Gradient {
    GradientColor topLeft, topRight, bottomLeft, bottomRight;

    Gradient() = default;

    /* Four corners in reading order: {topLeft, topRight, bottomLeft,
       bottomRight}. */
    Gradient(GradientColor tl, GradientColor tr, GradientColor bl, GradientColor br)
        : topLeft(std::move(tl)), topRight(std::move(tr)),
          bottomLeft(std::move(bl)), bottomRight(std::move(br)) {}

    /* Two colors = vertical fade (top -> bottom). */
    Gradient(GradientColor top, GradientColor bottom)
        : topLeft(top), topRight(std::move(top)),
          bottomLeft(bottom), bottomRight(std::move(bottom)) {}

    /* Fluent, position-named setters. */
    Gradient& setTopLeft(GradientColor c)     { topLeft = std::move(c);     return *this; }
    Gradient& setTopRight(GradientColor c)    { topRight = std::move(c);    return *this; }
    Gradient& setBottomLeft(GradientColor c)  { bottomLeft = std::move(c);  return *this; }
    Gradient& setBottomRight(GradientColor c) { bottomRight = std::move(c); return *this; }

    Gradient& setTop(GradientColor c)    { topLeft = c;    topRight = std::move(c);    return *this; }
    Gradient& setBottom(GradientColor c) { bottomLeft = c; bottomRight = std::move(c); return *this; }
    Gradient& setLeft(GradientColor c)   { topLeft = c;    bottomLeft = std::move(c);  return *this; }
    Gradient& setRight(GradientColor c)  { topRight = c;   bottomRight = std::move(c); return *this; }

    static Gradient vertical(GradientColor top, GradientColor bottom);
    static Gradient horizontal(GradientColor left, GradientColor right);

    /* False for a default-constructed Gradient (all corners unset) — the
       element falls back to its solid color. */
    bool active() const {
        const GradientColor unset;
        return topLeft != unset || topRight != unset ||
               bottomLeft != unset || bottomRight != unset;
    }

    /* Resolve the four corners for drawing (role colors through the palette,
       literal colors pass through). Output order: TL, TR, BL, BR. */
    void resolve(const Palette& palette, Color out[4]) const;

    bool operator==(const Gradient& o) const {
        return topLeft == o.topLeft && topRight == o.topRight &&
               bottomLeft == o.bottomLeft && bottomRight == o.bottomRight;
    }
    bool operator!=(const Gradient& o) const { return !(*this == o); }
};

} // namespace uilo
