#include "Resizer.hpp"
#include "../../UILO.hpp"

#include <algorithm>

namespace uilo {

Resizer::Resizer(Modifier modifier, ResizerOptions options, const std::string& name)
    : m_options(options)
{
    m_modifier = modifier;
    m_name = name;
    m_type = ElementType::Resizer;
}

void Resizer::update(sf::FloatRect& parentBounds, float dt) {
    (void)dt;
    m_bounds = parentBounds;  // Container sets exact bounds; no resize() here

    if (!m_uiloRef || !m_target || !m_dragging) return;

    if (!sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
        m_dragging = false;
        return;
    }

    const float scale          = m_uiloRef->getScale();
    const sf::Vector2f mouse   = m_uiloRef->getMousePosition();
    const ResizerDir dir       = m_options.getDirection();
    const bool isHoriz         = (dir == ResizerDir::Left || dir == ResizerDir::Right);

    m_uiloRef->requestCursor(
        isHoriz ? sf::Cursor::Type::SizeHorizontal : sf::Cursor::Type::SizeVertical, 2);

    const float dx      = (mouse.x - m_dragStart.x) / scale;
    const float dy      = (mouse.y - m_dragStart.y) / scale;
    const float parentW = m_containerBounds.size.x / scale;
    const float parentH = m_containerBounds.size.y / scale;

    switch (dir) {
        case ResizerDir::Left: {
            float wMin = std::max(1.f, m_options.getResizeWidthMin().resolve(parentW));
            float wMax = m_options.getResizeWidthMax().resolve(parentW);
            m_target->getModifier().setWidth({std::clamp(m_dragStartW + dx, wMin, wMax), false});
            break;
        }
        case ResizerDir::Right: {
            float wMin = std::max(1.f, m_options.getResizeWidthMin().resolve(parentW));
            float wMax = m_options.getResizeWidthMax().resolve(parentW);
            m_target->getModifier().setWidth({std::clamp(m_dragStartW - dx, wMin, wMax), false});
            break;
        }
        case ResizerDir::Top: {
            float hMin = std::max(1.f, m_options.getResizeHeightMin().resolve(parentH));
            float hMax = m_options.getResizeHeightMax().resolve(parentH);
            m_target->getModifier().setHeight({std::clamp(m_dragStartH + dy, hMin, hMax), false});
            break;
        }
        case ResizerDir::Bottom: {
            float hMin = std::max(1.f, m_options.getResizeHeightMin().resolve(parentH));
            float hMax = m_options.getResizeHeightMax().resolve(parentH);
            m_target->getModifier().setHeight({std::clamp(m_dragStartH - dy, hMin, hMax), false});
            break;
        }
    }
}

void Resizer::render(sf::RenderTarget& target) {
    if (!m_modifier.getVisible()) return;
    sf::Color c = m_options.getColor();
    if (c.a == 0) return;

    // Visual strip: getThickness() * scale, centered within the (larger) hit bounds.
    const float scale        = m_uiloRef ? m_uiloRef->getScale() : 1.f;
    const float visualThick  = m_options.getThickness() * scale;
    const bool  isHoriz      = (m_options.getDirection() == ResizerDir::Left ||
                                m_options.getDirection() == ResizerDir::Right);

    sf::FloatRect vis = m_bounds;
    if (isHoriz) {
        vis.position.x = m_bounds.position.x + (m_bounds.size.x - visualThick) * 0.5f;
        vis.size.x     = visualThick;
    } else {
        vis.position.y = m_bounds.position.y + (m_bounds.size.y - visualThick) * 0.5f;
        vis.size.y     = visualThick;
    }

    // Set a pixel-accurate view so the visual rect maps 1:1 to screen coords,
    // then restore whatever view was active before.
    const auto winSize   = target.getSize();
    const sf::View saved = target.getView();
    target.setView(sf::View(sf::FloatRect{
        {0.f, 0.f},
        {static_cast<float>(winSize.x), static_cast<float>(winSize.y)}
    }));

    sf::RectangleShape rect(vis.size);
    rect.setPosition(vis.position);
    rect.setFillColor(c);
    target.draw(rect);

    target.setView(saved);
}

bool Resizer::checkHover(const sf::Vector2f& mousePosition) {
    if (!m_bounds.contains(mousePosition)) return false;
    if (m_uiloRef) {
        const ResizerDir dir  = m_options.getDirection();
        const bool isHoriz    = (dir == ResizerDir::Left || dir == ResizerDir::Right);
        m_uiloRef->requestCursor(
            isHoriz ? sf::Cursor::Type::SizeHorizontal : sf::Cursor::Type::SizeVertical, 2);
    }
    return true;
}

bool Resizer::checkLeftClick(const sf::Vector2f& mousePosition) {
    if (!m_bounds.contains(mousePosition) || !m_uiloRef || !m_target) return false;

    const float scale = m_uiloRef->getScale();
    const float padU  = m_target->getModifier().getOuterPadding();
    m_dragStartW      = m_target->getBounds().size.x / scale + 2.f * padU;
    m_dragStartH      = m_target->getBounds().size.y / scale + 2.f * padU;
    m_dragStart       = mousePosition;
    m_dragging        = true;
    m_uiloRef->setCurrInteractible(this);
    return true;
}

void Resizer::onDeactivate() {
    m_dragging = false;
}

} // namespace uilo
