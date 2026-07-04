#pragma once
#include "../../utils/Math.hpp"
#include "../../utils/Color.hpp"

namespace uilo {

struct Rect {
    Vec2f position;
    Vec2f size;
    Color fillColor       = Color::White;
    Color outlineColor    = Color::Transparent;
    float outlineThickness = 0.f;

    // Optional per-corner fill (bilinear gradient). When `gradient` is true
    // the corner colors replace fillColor for the body; the outline is
    // unaffected. Set via setGradientColors (order: TL, TR, BL, BR).
    bool  gradient = false;
    Color colorTL, colorTR, colorBL, colorBR;

    void setGradientColors(const Color c[4]) {
        gradient = true;
        colorTL = c[0]; colorTR = c[1]; colorBL = c[2]; colorBR = c[3];
    }
};

} // namespace uilo
