#pragma once

#include "../utils/Vec2.hpp"
#include "../utils/Color.hpp"

namespace uilo {

enum class ShapeType { Rect, Circle, Line };

struct Shape {
    Vec2f position          = {0, 0};
    Color fillColor         = {255, 255, 255, 255};
    Color outlineColor      = {0, 0, 0, 0};
    float outlineThickness  = 0.f;

    virtual ~Shape() = default;
    virtual ShapeType type() const = 0;
    virtual bool containsPoint(const Vec2f& point) const = 0;
};

struct Rect : Shape {
    Vec2f size              = {0, 0};
    float cornerRadius      = 0.f;

    ShapeType type() const override { return ShapeType::Rect; }

    bool containsPoint(const Vec2f& point) const override {
        return point.x >= position.x && point.x <= position.x + size.x
            && point.y >= position.y && point.y <= position.y + size.y;
    }
};

struct Circle : Shape {
    float radius            = 0.f;

    ShapeType type() const override { return ShapeType::Circle; }

    bool containsPoint(const Vec2f& point) const override {
        float dx = point.x - position.x;
        float dy = point.y - position.y;
        return (dx * dx + dy * dy) <= (radius * radius);
    }
};

struct Line : Shape {
    Vec2f end               = {0, 0};
    float thickness         = 1.f;

    ShapeType type() const override { return ShapeType::Line; }

    bool containsPoint(const Vec2f& /*point*/) const override { return false; }
};

}