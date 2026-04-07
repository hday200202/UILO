#pragma once

#include <cmath>
#include <vector>
#include "Vertex.hpp"

namespace uilo {

class Shape {
public:
    Shape() = default;
    virtual ~Shape() = default;

    const std::vector<Vertex>& getVertices() const { return m_verts; }
    const std::vector<uint32_t>& getIndices() const { return m_indices; }

    const Vec2f& getPosition() const { return m_position; }
    const Vec2f& getSize()     const { return m_size; }
    const Color& getColor()    const { return m_color; }

    void setPosition(const Vec2f& position) { m_position = position; updateVertices(); }
    void setSize(const Vec2f& size) { m_size = size; updateVertices(); }
    void setColor(const Color& color) { m_color = color; updateVertices(); }

protected:
    Vec2f m_position        = {0, 0};
    Vec2f m_size            = {0, 0};
    Color m_color           = {255, 255, 255, 255};

    std::vector<Vertex>     m_verts;
    std::vector<uint32_t>   m_indices;

    virtual void updateVertices() = 0;
};

class Rect : public Shape {
public:
    void setCornerRadius(float radius) { m_cornerRadius = radius; updateVertices(); }

private:
    float m_cornerRadius = 0;

    void updateVertices() override {
        m_verts.clear();
        m_indices.clear();

        float x = m_position.x;
        float y = m_position.y;
        float w = m_size.x;
        float h = m_size.y;

        if (m_cornerRadius == 0) {
            m_verts.push_back({{x,      y + h}, m_color});
            m_verts.push_back({{x,      y    }, m_color});
            m_verts.push_back({{x + w,  y    }, m_color});
            m_verts.push_back({{x + w,  y + h}, m_color});

            m_indices = {0, 1, 2, 0, 2, 3};
        }

        else {
            float r = m_cornerRadius;
            if (r > w * 0.5f) r = w * 0.5f;
            if (r > h * 0.5f) r = h * 0.5f;

            const int segments = 8;
            const float halfPI = 3.14159265f * 0.5f;

            m_verts.push_back({{x + w * 0.5f, y + h * 0.5f}, m_color});

            float cx[] = { x + r,     x + w - r, x + w - r, x + r     };
            float cy[] = { y + r,     y + r,     y + h - r, y + h - r };
            float startAngle[] = { 2 * halfPI, 3 * halfPI, 0, halfPI };

            for (int corner = 0; corner < 4; corner++) {
                for (int i = 0; i <= segments; i++) {
                    float angle = startAngle[corner] + halfPI * i / segments;
                    float vx = cx[corner] + std::cos(angle) * r;
                    float vy = cy[corner] + std::sin(angle) * r;
                    m_verts.push_back({{vx, vy}, m_color});
                }
            }

            uint32_t totalOuter = 4 * (segments + 1);
            for (uint32_t i = 1; i <= totalOuter; i++) {
                uint32_t next = (i % totalOuter) + 1;
                m_indices.push_back(0);
                m_indices.push_back(i);
                m_indices.push_back(next);
            }
        }
    }
};

class Circle : public Shape {
public:
    void setRadius(float radius) { m_radius = radius; updateVertices(); }
    void setSegments(int segments) { m_segments = segments; updateVertices(); }

private:
    float m_radius = 1;
    int m_segments = 32;

    void updateVertices() override {
        m_verts.clear();
        m_indices.clear();

        float cx = m_position.x;
        float cy = m_position.y;
        const float PI2 = 3.14159265f * 2.0f;

        m_verts.push_back({{cx, cy}, m_color});

        for (int i = 0; i < m_segments; i++) {
            float angle = PI2 * i / m_segments;
            float vx = cx + std::cos(angle) * m_radius;
            float vy = cy + std::sin(angle) * m_radius;
            m_verts.push_back({{vx, vy}, m_color});
        }

        for (int i = 1; i <= m_segments; i++) {
            int next = (i % m_segments) + 1;
            m_indices.push_back(0);
            m_indices.push_back(i);
            m_indices.push_back(next);
        }
    }
};

class Line : public Shape {
public:
    void setStart(const Vec2f& start) { m_start = start; updateVertices(); }
    void setEnd(const Vec2f& end) { m_end = end; updateVertices(); }
    void setThickness(float thickness) { m_thickness = thickness; updateVertices(); }

private:
    Vec2f m_start = {0, 0};
    Vec2f m_end   = {0, 0};
    float m_thickness = 1.0f;

    void updateVertices() override {
        m_verts.clear();
        m_indices.clear();

        Vec2f dir = m_end.sub(m_start);
        float len = dir.magnitude();
        if (len == 0) return;

        Vec2f perp = {-dir.y / len, dir.x / len};
        float half = m_thickness * 0.5f;
        perp = perp.mul(half);

        m_verts.push_back({{m_start.x + perp.x, m_start.y + perp.y}, m_color});
        m_verts.push_back({{m_start.x - perp.x, m_start.y - perp.y}, m_color});
        m_verts.push_back({{m_end.x   - perp.x, m_end.y   - perp.y}, m_color});
        m_verts.push_back({{m_end.x   + perp.x, m_end.y   + perp.y}, m_color});

        m_indices = {0, 1, 2, 0, 2, 3};
    }
};

} // namespace uilo