#pragma once
#include "../../utils/Math.hpp"
#include "../../utils/Color.hpp"

namespace uilo {

struct Line {
    Vec2f start;
    Vec2f end;
    float thickness = 1.f;
    Color color     = Color::White;
};

} // namespace uilo
