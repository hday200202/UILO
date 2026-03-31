#pragma once

#include "../math/Vec2.hpp"

namespace uilo {

struct Input {
    bool leftMouse = false;
    bool rightMouse = false;
    float scrollDelta = 0.f;

    Vec2f mousePosition = {0, 0};

    inline void reset() {
        leftMouse = false;
        rightMouse = false;
        scrollDelta = 0.f;
    }
};

}