#pragma once

#include <cmath>
#include "../utils/Vec2.hpp"

namespace uilo {

struct Bounds {
    Vec2f position  = {0, 0};
    Vec2f size      = {0, 0};

    float left()   const { return position.x; }
    float top()    const { return position.y; }
    float right()  const { return position.x + size.x; }
    float bottom() const { return position.y + size.y; }
    float width()  const { return size.x; }
    float height() const { return size.y; }

    Vec2f center() const {
        return {position.x + size.x / 2.f, position.y + size.y / 2.f};
    }

    bool contains(const Vec2f& point) const {
        return point.x >= left() && point.x <= right()
            && point.y >= top()  && point.y <= bottom();
    }

    bool intersects(const Bounds& other) const {
        return left() < other.right()  && right()  > other.left()
            && top()  < other.bottom() && bottom() > other.top();
    }

    Bounds intersection(const Bounds& other) const {
        float x1 = std::max(left(),   other.left());
        float y1 = std::max(top(),    other.top());
        float x2 = std::min(right(),  other.right());
        float y2 = std::min(bottom(), other.bottom());
        if (x2 <= x1 || y2 <= y1) return {};
        return {{x1, y1}, {x2 - x1, y2 - y1}};
    }

    Bounds padded(float padding) const {
        return {{position.x + padding, position.y + padding},
                {size.x - 2 * padding, size.y - 2 * padding}};
    }

    Bounds expanded(float amount) const {
        return {{position.x - amount, position.y - amount},
                {size.x + 2 * amount, size.y + 2 * amount}};
    }
};

}
