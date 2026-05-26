#pragma once
#include "../../utils/Math.hpp"
#include "../../utils/Color.hpp"

namespace uilo {

struct Circle {
    Vec2f center;
    float radius           = 0.f;
    int   segments         = 32;    // higher = smoother; 32 is fine for most UI
    Color fillColor        = Color::White;
    Color outlineColor     = Color::Transparent;
    float outlineThickness = 0.f;
};

} // namespace uilo
