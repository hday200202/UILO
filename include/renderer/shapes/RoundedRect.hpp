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
};

} // namespace uilo
