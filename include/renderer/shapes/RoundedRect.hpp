#pragma once
#include "../../utils/Math.hpp"
#include "../../utils/Color.hpp"

namespace uilo {

struct RoundedRect {
    Vec2f position;
    Vec2f size;
    float radius           = 0.f;
    int   cornerSegments   = 8;     // arc samples per corner; higher = smoother
    Color fillColor        = Color::White;
    Color outlineColor     = Color::Transparent;
    float outlineThickness = 0.f;

    // Optional per-corner fill (bilinear gradient). When `gradient` is true
    // the corner colors replace fillColor for the body; the outline is
    // unaffected. The SDF corner mask applies to the gradient exactly as it
    // does to a solid fill. Set via setGradientColors (order: TL, TR, BL, BR).
    bool  gradient = false;
    Color colorTL = Color::White, colorTR = Color::White,
          colorBL = Color::White, colorBR = Color::White;

    void setGradientColors(const Color c[4]) {
        gradient = true;
        colorTL = c[0]; colorTR = c[1]; colorBL = c[2]; colorBR = c[3];
    }
};

} // namespace uilo
