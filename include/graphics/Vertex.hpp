#pragma once

#include "../math/Vec2.hpp"
#include "Color.hpp"

namespace uilo {

struct Vertex {
    Vec2f position = {0, 0};
    Color color = {255, 255, 255, 255};
    Vec2f texCoord = {0, 0};
};

}