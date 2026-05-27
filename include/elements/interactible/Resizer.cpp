#include "Resizer.hpp"
#include "../../UILO.hpp"
#include <SDL3/SDL.h>
#include <algorithm>

namespace uilo {

Resizer::Resizer(Modifier modifier, ResizerOptions options, const std::string& name)
    : m_options(options)
{
    m_modifier = modifier;
    m_name = name;
    m_type = ElementType::Resizer;
}

void Resizer::update(Rectf& parentBounds, float dt) {
    (void)dt;
    m_bounds = parentBounds;  // Container sets exact bounds; no resize() here

    if (!m_uiloRef || !m_target || !m_dragging) return;

    float mx, my;
    uint32_t btns = SDL_GetMouseState(&mx, &my);
    if (!(btns & SDL_BUTTON_MASK(SDL_BUTTON_LEFT))) {
        m_dragging = false;
        return;
    }

    const float scale          = m_uiloRef->getScale();
    const Vec2f mouse   = m_uiloRef->getMousePosition();
    const ResizerDir dir       = m_options.getDirection();
    const bool isHoriz         = (dir == ResizerDir::Left || dir == ResizerDir::Right);

    m_uiloRef->requestCursor(
        isHoriz ? CursorType::SizeHorizontal : CursorType::SizeVertical, 2);

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

void Resizer::render() {
    if (!m_modifier.getVisible()) return;
    // TODO: BGFX rendering for resizer visual strip
    Color c = m_options.getColor();
    if (c.a == 0) { m_dirty = false; return; }
    if (m_uiloRef) {
        const float scale       = m_uiloRef->getScale();
        const float visualThick = m_options.getThickness() * scale;
        const bool  isHoriz     = (m_options.getDirection() == ResizerDir::Left ||
                                   m_options.getDirection() == ResizerDir::Right);
        Rectf vis = m_bounds;
        if (isHoriz) {
            vis.position.x = m_bounds.position.x + (m_bounds.size.x - visualThick) * 0.5f;
            vis.size.x     = visualThick;
        } else {
            vis.position.y = m_bounds.position.y + (m_bounds.size.y - visualThick) * 0.5f;
            vis.size.y     = visualThick;
        }
        m_uiloRef->getRenderer().draw(Rect{vis.position, vis.size, c});
    }
    m_dirty = false;
}

bool Resizer::checkHover(const Vec2f& mousePosition) {
    if (!m_bounds.contains(mousePosition)) {
        if (m_hovered) {
            m_hovered = false; m_dirty = true;
            if (m_modifier.getOnHoverExit()) m_modifier.getOnHoverExit()(this);
        }
        return false;
    }
    if (!m_hovered) {
        m_hovered = true; m_dirty = true;
        if (m_modifier.getOnHoverEnter()) m_modifier.getOnHoverEnter()(this);
    }
    if (m_uiloRef) {
        const ResizerDir dir = m_options.getDirection();
        const bool isHoriz   = (dir == ResizerDir::Left || dir == ResizerDir::Right);
        m_uiloRef->requestCursor(
            isHoriz ? CursorType::SizeHorizontal : CursorType::SizeVertical, 2);
    }
    return true;
}

bool Resizer::checkLeftClick(const Vec2f& mousePosition) {
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
