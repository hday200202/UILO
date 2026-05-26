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
};

} // namespace uilo
