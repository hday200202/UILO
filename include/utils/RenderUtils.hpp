#pragma once

#include <cmath>
#include <algorithm>

#include <SFML/Graphics.hpp>

namespace uilo {

/*
    Appends a solid-color axis-aligned quad (two triangles) to a VertexArray.
    The VertexArray must use PrimitiveType::Triangles.
*/
inline void appendQuad(sf::VertexArray& va, sf::Vector2f pos, sf::Vector2f size, sf::Color color) {
    va.append({{pos.x,          pos.y         }, color});
    va.append({{pos.x + size.x, pos.y         }, color});
    va.append({{pos.x + size.x, pos.y + size.y}, color});
    va.append({{pos.x,          pos.y         }, color});
    va.append({{pos.x + size.x, pos.y + size.y}, color});
    va.append({{pos.x,          pos.y + size.y}, color});
}

/*
    Computes a clipping sf::View that maps boundsInWorld to a sub-viewport of
    parentView. Works correctly whether target is a window or a RenderTexture.
    Use: save view, setView(computeClipView(...)), draw children, restore view.
*/
inline sf::View computeClipView(const sf::View& parentView, const sf::FloatRect& boundsInWorld) {
    const sf::Vector2f vs  = parentView.getSize();
    const sf::Vector2f vtl = parentView.getCenter() - vs * 0.5f;
    const sf::FloatRect vp = parentView.getViewport();

    sf::View clip;
    clip.setCenter(boundsInWorld.position + boundsInWorld.size * 0.5f);
    clip.setSize(boundsInWorld.size);
    clip.setViewport(sf::FloatRect{
        { vp.position.x + (boundsInWorld.position.x - vtl.x) / vs.x * vp.size.x,
          vp.position.y + (boundsInWorld.position.y - vtl.y) / vs.y * vp.size.y },
        { boundsInWorld.size.x / vs.x * vp.size.x,
          boundsInWorld.size.y / vs.y * vp.size.y }
    });
    return clip;
}


/*
    Builds a convex polygon that approximates a rounded rectangle.
    segs = arc sample count per corner (>= 2).
    Corners are ordered: top-right, bottom-right, bottom-left, top-left.
*/
inline sf::ConvexShape makeRoundedRect(sf::Vector2f pos, sf::Vector2f size, float r, int segs = 8) {
    segs = std::max(2, segs);
    r    = std::min(r, std::min(size.x, size.y) * 0.5f);

    static constexpr float kDeg2Rad = 3.14159265f / 180.f;

    struct Corner { float cx, cy, startDeg, endDeg; };
    const Corner corners[4] = {
        { size.x - r, r,            -90.f,   0.f },  // top-right
        { size.x - r, size.y - r,     0.f,  90.f },  // bottom-right
        { r,          size.y - r,    90.f, 180.f },   // bottom-left
        { r,          r,            180.f, 270.f },   // top-left
    };

    const auto pointCount = static_cast<std::size_t>(4 * segs);
    sf::ConvexShape shape(pointCount);

    std::size_t idx = 0;
    for (const auto& c : corners) {
        for (int i = 0; i < segs; ++i) {
            float t     = static_cast<float>(i) / (segs - 1);
            float angle = (c.startDeg + t * (c.endDeg - c.startDeg)) * kDeg2Rad;
            shape.setPoint(idx++, {
                pos.x + c.cx + r * std::cos(angle),
                pos.y + c.cy + r * std::sin(angle)
            });
        }
    }

    return shape;
}

/*
    Erases the four corner regions of `rt` that lie outside the rounded
    rectangle described by `bounds` and radius `r`.  The erase blend mode
    zeros destination alpha while leaving the colour channels intact, so
    the corners become fully transparent.
*/
inline void eraseCorners(sf::RenderTexture& rt, sf::FloatRect bounds, float r, int segs = 8) {
    segs = std::max(2, segs);
    r    = std::min(r, std::min(bounds.size.x, bounds.size.y) * 0.5f);

    static constexpr float kDeg2Rad = 3.14159265f / 180.f;

    // Erase blend: keep dst colour, zero dst alpha
    static const sf::BlendMode kErase(
        sf::BlendMode::Factor::Zero, sf::BlendMode::Factor::One,  sf::BlendMode::Equation::Add,
        sf::BlendMode::Factor::Zero, sf::BlendMode::Factor::Zero, sf::BlendMode::Equation::Add
    );

    const float l = bounds.position.x;
    const float t = bounds.position.y;
    const float w = bounds.size.x;
    const float h = bounds.size.y;

    struct Corner { float fanX, fanY, arcCX, arcCY, startDeg, endDeg; };
    const Corner corners[4] = {
        { l,     t,     l + r,     t + r,     270.f, 180.f },  // top-left
        { l + w, t,     l + w - r, t + r,     270.f, 360.f },  // top-right
        { l + w, t + h, l + w - r, t + h - r,   0.f,  90.f },  // bottom-right
        { l,     t + h, l + r,     t + h - r,  90.f, 180.f },  // bottom-left
    };

    for (const auto& c : corners) {
        sf::VertexArray va(sf::PrimitiveType::TriangleFan,
                           static_cast<std::size_t>(segs + 2));

        va[0].position = { c.fanX, c.fanY };
        va[0].color    = sf::Color::Black;

        for (int i = 0; i <= segs; ++i) {
            float s     = static_cast<float>(i) / segs;
            float angle = (c.startDeg + s * (c.endDeg - c.startDeg)) * kDeg2Rad;
            va[static_cast<std::size_t>(i + 1)].position = {
                c.arcCX + r * std::cos(angle),
                c.arcCY + r * std::sin(angle)
            };
            va[static_cast<std::size_t>(i + 1)].color = sf::Color::Black;
        }

        rt.draw(va, sf::RenderStates(kErase));
    }
}

} // namespace uilo
