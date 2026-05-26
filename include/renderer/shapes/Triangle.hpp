#pragma once
#include "../../utils/Math.hpp"
#include "../../utils/Color.hpp"

namespace uilo {

struct Triangle {
    Vec2f a, b, c;
    Color fillColor        = Color::White;
    Color outlineColor     = Color::Transparent;
    float outlineThickness = 0.f;
};

} // namespace uilo
