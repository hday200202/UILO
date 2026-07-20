#pragma once

#include "Color.hpp"

#include <string>
#include <utility>

namespace uilo {

class Palette; // resolve() walks it at draw time — see Gradient.cpp

/*
    Gradient — a per-corner background fill, built once and shared across
    elements the same way a Material is.

    A gradient is made of four corner colors. Each corner is a GradientColor:
    either a literal Color or a palette role (resolved at draw time, so
    switching palettes restyles gradients automatically). The two are freely
    mixable, and because GradientColor converts implicitly from both a Color
    and a role string, you never name the type directly:

        RowOptions().setGradient({tl, tr, bl, br});                        // literal colors
        RowOptions().setGradient({"accent", "accent", "panel", "panel"});  // palette roles
        RowOptions().setGradient({Color::Red, "accent", ...});             // mixed

    The fluent setters name the position each color occupies, so a gradient
    reads left-to-right without remembering corner order:

        Gradient().setTop(Color{60, 40, 120}).setBottom(Color{20, 20, 40}); // vertical fade
        Gradient().setLeft("accent").setRight("panel");                     // horizontal, roles
        Gradient().setTopLeft(a).setTopRight(b)
                  .setBottomLeft(c).setBottomRight(d);                      // four corners

    The vertical()/horizontal() factories and the positional constructors
    remain as shorthand. Whole gradients can also be named in the Palette
    (palette.setGradient("hero", g)) and referenced per element with
    setGradientRole("hero").

    Colors are interpolated across the background quad on the GPU, and the
    rounded-corner SDF mask clips the result exactly like a solid fill —
    gradients and rounding compose with no extra cost.
*/

// One corner of a gradient: a literal Color, or a palette role resolved at
// draw time. Converts implicitly from Color and from a role string, so call
// sites just pass a color or a role name.
struct GradientColor {
    Color       color { 0, 0, 0, 0 };
    std::string role;                 // non-empty -> resolved via palette

    GradientColor() = default;
    GradientColor(Color c) : color(c) {}
    GradientColor(const char* paletteRole) : role(paletteRole) {}
    GradientColor(std::string paletteRole) : role(std::move(paletteRole)) {}

    bool operator==(const GradientColor& o) const { return color == o.color && role == o.role; }
    bool operator!=(const GradientColor& o) const { return !(*this == o); }
};

// Deprecated alias for the old name. Prefer GradientColor.
using GradientStop = GradientColor;

struct Gradient {
    GradientColor topLeft, topRight, bottomLeft, bottomRight;

    Gradient() = default;

    // Four corners in reading order: {topLeft, topRight, bottomLeft, bottomRight}.
    Gradient(GradientColor tl, GradientColor tr, GradientColor bl, GradientColor br)
        : topLeft(std::move(tl)), topRight(std::move(tr)),
          bottomLeft(std::move(bl)), bottomRight(std::move(br)) {}

    // Two colors = vertical fade (top -> bottom).
    Gradient(GradientColor top, GradientColor bottom)
        : topLeft(top), topRight(std::move(top)),
          bottomLeft(bottom), bottomRight(std::move(bottom)) {}

    // Fluent, position-named setters. Each returns *this so they chain, and
    // the name states where the color goes. The edge setters (setTop, etc.)
    // set both corners of that edge.
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

    // False for a default-constructed Gradient (all corners unset) — the
    // element falls back to its solid color. A gradient of deliberately
    // transparent literals still counts as inactive because it would draw
    // nothing anyway.
    bool active() const {
        const GradientColor unset;
        return topLeft != unset || topRight != unset ||
               bottomLeft != unset || bottomRight != unset;
    }

    // Resolve the four corners for drawing (role colors through the palette,
    // literal colors pass through). Output order: TL, TR, BL, BR.
    void resolve(const Palette& palette, Color out[4]) const;

    bool operator==(const Gradient& o) const {
        return topLeft == o.topLeft && topRight == o.topRight &&
               bottomLeft == o.bottomLeft && bottomRight == o.bottomRight;
    }
    bool operator!=(const Gradient& o) const { return !(*this == o); }
};

} // namespace uilo
