#pragma once

#include <cmath>
#include <cstdint>

namespace uilo {

struct Vec2f {
    float x = 0.f;
    float y = 0.f;

    Vec2f(float x, float y) : x(x), y(y) {}

    float magnitude()       const { return std::sqrt((x*x) + (y*y)); }
    Vec2f mul(float scalar) const { return {x * scalar, y * scalar}; }
    Vec2f add(const Vec2f& other) const { return {x + other.x, y + other.y}; }
    Vec2f sub(const Vec2f& other) const { return {x - other.x, y - other.y}; }

    Vec2f normalized() const {
        float mag = magnitude();
        if (mag == 0) return {0, 0};
        return mul(1.f / mag); 
    }
};

struct Vec2i {
    int32_t x = 0;
    int32_t y = 0;

    Vec2i(int32_t x, int32_t y) : x(x), y(y) {}

    float magnitude() const { return std::sqrt(float(x*x) + float(y*y)); }
    Vec2i mul(int32_t scalar) const { return {x * scalar, y * scalar}; }
    Vec2i add(const Vec2i& other) const { return {x + other.x, y + other.y}; }
    Vec2i sub(const Vec2i& other) const { return {x - other.x, y - other.y}; }

    Vec2f normalized() const {
        float mag = magnitude();
        if (mag == 0) return {0.f, 0.f};
        return {x / mag, y / mag};
    }
};

struct Vec2u {
    uint32_t x = 0;
    uint32_t y = 0;

    Vec2u(uint32_t x, uint32_t y) : x(x), y(y) {}

    float magnitude() const { return std::sqrt(float(x*x) + float(y*y)); }
    Vec2u mul(uint32_t scalar) const { return {x * scalar, y * scalar}; }
    Vec2u add(const Vec2u& other) const { return {x + other.x, y + other.y}; }
    Vec2u sub(const Vec2u& other) const { return {x - other.x, y - other.y}; }

    Vec2f normalized() const {
        float mag = magnitude();
        if (mag == 0) return {0.f, 0.f};
        return {x / mag, y / mag};
    }
};

}