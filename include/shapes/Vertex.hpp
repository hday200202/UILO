#pragma once

#include "../utils/Vec2.hpp"
#include "../utils/Color.hpp"

namespace uilo {

struct Vertex {
    Vec2f position  = {0, 0};
    Color color     = {255, 255, 255, 255};
};

}