#pragma once
#include <cmath>
#include <cstdint>
#include "Color.hpp"

namespace uilo {

struct Vec2f {
    float x = 0.f, y = 0.f;

    constexpr Vec2f() = default;
    constexpr Vec2f(float x, float y) : x(x), y(y) {}

    constexpr Vec2f  operator+(const Vec2f& o) const { return {x + o.x, y + o.y}; }
    constexpr Vec2f  operator-(const Vec2f& o) const { return {x - o.x, y - o.y}; }
    constexpr Vec2f  operator*(float s)        const { return {x * s,   y * s};   }
    constexpr Vec2f  operator/(float s)        const { return {x / s,   y / s};   }
    constexpr Vec2f& operator+=(const Vec2f& o)      { x += o.x; y += o.y; return *this; }
    constexpr Vec2f& operator-=(const Vec2f& o)      { x -= o.x; y -= o.y; return *this; }
    constexpr Vec2f& operator*=(float s)             { x *= s;   y *= s;   return *this; }
    constexpr bool   operator==(const Vec2f& o) const { return x == o.x && y == o.y; }
    constexpr bool   operator!=(const Vec2f& o) const { return !(*this == o); }

    float length()  const { return std::sqrt(x * x + y * y); }
};

inline Vec2f operator*(float s, const Vec2f& v) { return {s * v.x, s * v.y}; }

struct Vec2u {
    unsigned x = 0u, y = 0u;

    constexpr Vec2u() = default;
    constexpr Vec2u(unsigned x, unsigned y) : x(x), y(y) {}

    constexpr bool operator==(const Vec2u& o) const { return x == o.x && y == o.y; }
    constexpr bool operator!=(const Vec2u& o) const { return !(*this == o); }
};

struct Vec2i {
    int x = 0, y = 0;

    constexpr Vec2i() = default;
    constexpr Vec2i(int x, int y) : x(x), y(y) {}

    constexpr bool operator==(const Vec2i& o) const { return x == o.x && y == o.y; }
    constexpr bool operator!=(const Vec2i& o) const { return !(*this == o); }
};

struct Rectf {
    Vec2f position;
    Vec2f size;

    constexpr Rectf() = default;
    constexpr Rectf(Vec2f position, Vec2f size) : position(position), size(size) {}
    constexpr Rectf(float x, float y, float w, float h)
        : position(x, y), size(w, h) {}

    constexpr float left()   const { return position.x; }
    constexpr float top()    const { return position.y; }
    constexpr float width()  const { return size.x; }
    constexpr float height() const { return size.y; }
    constexpr float right()  const { return position.x + size.x; }
    constexpr float bottom() const { return position.y + size.y; }

    constexpr bool contains(Vec2f p) const {
        return p.x >= position.x && p.x < position.x + size.x &&
               p.y >= position.y && p.y < position.y + size.y;
    }

    constexpr bool operator==(const Rectf& o) const {
        return position == o.position && size == o.size;
    }
    constexpr bool operator!=(const Rectf& o) const { return !(*this == o); }
};

} // namespace uilo
