#pragma once

#include "../math/Vec2.hpp"

namespace uilo {

struct Bounds {
    Vec2f position = {0, 0};
    Vec2f size = {0, 0};

    // Edges
    float left()   const { return position.x; }
    float right()  const { return position.x + size.x; }
    float top()    const { return position.y; }
    float bottom() const { return position.y + size.y; }

    // Corners
    Vec2f topLeft()     const { return position; }
    Vec2f topRight()    const { return {right(), top()}; }
    Vec2f bottomLeft()  const { return {left(), bottom()}; }
    Vec2f bottomRight() const { return {right(), bottom()}; }

    // Center
    Vec2f center() const { return {position.x + size.x * 0.5f, position.y + size.y * 0.5f}; }

    bool intersects(const Bounds& other) const {
        return left() < other.right()  && right()  > other.left()
            && top()  < other.bottom() && bottom() > other.top();
    }

    bool intersects(const Vec2f& point) const {
        return left() < point.x && right()  > point.x
            && top()  < point.y && bottom() > point.y;
    }
};

}