#pragma once

#include "Color.hpp"

#include <string>
#include <utility>

namespace uilo {

class Palette; // resolve() walks it at draw time — see Gradient.cpp

/*
    Gradient — a per-corner background fill, built once and shared across
    elements the same way a Material is:

        Gradient g = Gradient::vertical(Color{60, 40, 120}, Color{20, 20, 40});
        row(Modifier(), RowOptions().setGradient(g), ...);

    Each corner is a GradientStop, which is either a literal Color or a
    palette role (resolved at draw time, so switching palettes restyles
    gradients automatically). The two are freely mixable:

        RowOptions().setGradient({tl, tr, bl, br});                      // literal colors
        RowOptions().setGradient({"accent", "accent", "panel", "panel"}); // palette roles
        RowOptions().setGradient({Color::Red, "accent", ...});            // mixed

    Two stops mean a vertical fade (top -> bottom); use the horizontal()
    factory for left -> right. Whole gradients can also be named in the
    Palette (palette.setGradient("hero", g)) and referenced per element
    with setGradientRole("hero").

    Colors are interpolated across the background quad on the GPU, and the
    rounded-corner SDF mask clips the result exactly like a solid fill —
    gradients and rounding compose with no extra cost.
*/

struct GradientStop {
    Color       color { 0, 0, 0, 0 };
    std::string role;                 // non-empty -> resolved via palette

    GradientStop() = default;
    GradientStop(Color c) : color(c) {}
    GradientStop(const char* paletteRole) : role(paletteRole) {}
    GradientStop(std::string paletteRole) : role(std::move(paletteRole)) {}

    bool operator==(const GradientStop& o) const { return color == o.color && role == o.role; }
    bool operator!=(const GradientStop& o) const { return !(*this == o); }
};

struct Gradient {
    GradientStop topLeft, topRight, bottomLeft, bottomRight;

    Gradient() = default;

    // Four corners in reading order: {topLeft, topRight, bottomLeft, bottomRight}.
    Gradient(GradientStop tl, GradientStop tr, GradientStop bl, GradientStop br)
        : topLeft(std::move(tl)), topRight(std::move(tr)),
          bottomLeft(std::move(bl)), bottomRight(std::move(br)) {}

    // Two stops = vertical fade (top -> bottom).
    Gradient(GradientStop top, GradientStop bottom)
        : topLeft(top), topRight(std::move(top)),
          bottomLeft(bottom), bottomRight(std::move(bottom)) {}

    static Gradient vertical(GradientStop top, GradientStop bottom);
    static Gradient horizontal(GradientStop left, GradientStop right);

    // False for a default-constructed Gradient (all stops unset) — the
    // element falls back to its solid color. A gradient of deliberately
    // transparent literals still counts as inactive because it would draw
    // nothing anyway.
    bool active() const {
        const GradientStop unset;
        return topLeft != unset || topRight != unset ||
               bottomLeft != unset || bottomRight != unset;
    }

    // Resolve the four corners for drawing (role stops through the palette,
    // literal stops pass through). Output order: TL, TR, BL, BR.
    void resolve(const Palette& palette, Color out[4]) const;

    bool operator==(const Gradient& o) const {
        return topLeft == o.topLeft && topRight == o.topRight &&
               bottomLeft == o.bottomLeft && bottomRight == o.bottomRight;
    }
    bool operator!=(const Gradient& o) const { return !(*this == o); }
};

} // namespace uilo
