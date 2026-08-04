#include "Resizer.hpp"
#include "../../UILO.hpp"
#include <SDL3/SDL.h>
#include <algorithm>
#include <cmath>

namespace uilo {

/*
    Resizer(Modifier modifier, ResizerOptions options, const std::string& name):
    - Params:   Modifier modifier, ResizerOptions options, const std::string&
                name
    - Returns:  Resizer
    - Desc:     Constructs a drag handle and tags it as a Resizer, which is what
                makes Row and Column skip it in the flow and place it at a
                boundary instead.
*/
Resizer::Resizer(
    Modifier modifier,
    ResizerOptions options,
    const std::string& name
) : m_options(options) {
    m_modifier = modifier;
    m_name = name;
    m_type = ElementType::Resizer;
}

/*
    update(Rectf& parentBounds, float dt):
    - Params:   Rectf& parentBounds, float dt
    - Returns:  void
    - Desc:     Takes the bounds the parent worked out and, while a drag is in
                progress, resizes the target. The parent's rect is used verbatim
                rather than run through resize(), because Row and Column place a
                resizer at a boundary rather than in the flow and have already
                decided exactly where it goes. The drag ends when the mouse
                button is found released, which is polled here rather than
                driven by an event so a release outside the window still
                finishes it. Deltas are converted out of render pixels into
                unscaled units before being applied, since a Modifier's
                dimensions are unscaled, and each direction clamps to its own
                min and max and quantises to the step when one is set.
*/
void Resizer::update(Rectf& parentBounds, float dt) {
    (void)dt;
    m_bounds = parentBounds;

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

    auto snapToStep = [](float value, float minValue, float maxValue, float step) {
        float v = std::clamp(value, minValue, maxValue);
        if (step <= 0.f) return v;
        const float snapped = minValue + std::round((v - minValue) / step) * step;
        return std::clamp(snapped, minValue, maxValue);
    };

    switch (dir) {
        case ResizerDir::Left: {
            float wMin = std::max(1.f, m_options.getResizeWidthMin().resolve(parentW));
            float wMax = m_options.getResizeWidthMax().resolve(parentW);
            float wStep = m_options.getResizeWidthStep().resolve(parentW);
            float w = snapToStep(m_dragStartW + dx, wMin, wMax, wStep);
            m_target->getModifier().setWidth({w, false});
            break;
        }
        case ResizerDir::Right: {
            float wMin = std::max(1.f, m_options.getResizeWidthMin().resolve(parentW));
            float wMax = m_options.getResizeWidthMax().resolve(parentW);
            float wStep = m_options.getResizeWidthStep().resolve(parentW);
            float w = snapToStep(m_dragStartW - dx, wMin, wMax, wStep);
            m_target->getModifier().setWidth({w, false});
            break;
        }
        case ResizerDir::Top: {
            float hMin = std::max(1.f, m_options.getResizeHeightMin().resolve(parentH));
            float hMax = m_options.getResizeHeightMax().resolve(parentH);
            float hStep = m_options.getResizeHeightStep().resolve(parentH);
            float h = snapToStep(m_dragStartH + dy, hMin, hMax, hStep);
            m_target->getModifier().setHeight({h, false});
            break;
        }
        case ResizerDir::Bottom: {
            float hMin = std::max(1.f, m_options.getResizeHeightMin().resolve(parentH));
            float hMax = m_options.getResizeHeightMax().resolve(parentH);
            float hStep = m_options.getResizeHeightStep().resolve(parentH);
            float h = snapToStep(m_dragStartH - dy, hMin, hMax, hStep);
            m_target->getModifier().setHeight({h, false});
            break;
        }
    }
}

/*
    render():
    - Params:   none
    - Returns:  void
    - Desc:     Draws the visible strip, which is narrower than the hit area and
                centred within it -- the handle is easy to grab but reads as a
                thin divider. Nothing is drawn when the colour is transparent,
                which is the default: a resizer is usually invisible until a
                hover handler colours it in.
*/
void Resizer::render() {
    if (!m_modifier.getVisible()) return;
    Color c = resolveColor(m_options.getColorRole(), m_options.getColor());
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

/*
    checkHover(const Vec2f& mousePosition):
    - Params:   const Vec2f& mousePosition
    - Returns:  bool -- true whenever the pointer is inside
    - Desc:     Tracks the hovered state, fires the enter and exit handlers, and
                asks for the resize cursor matching the drag axis. The request
                is made at a higher priority than an ordinary element's, so the
                resize arrows win over the hand of whatever the handle
                straddles.
*/
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

/*
    checkLeftClick(const Vec2f& mousePosition):
    - Params:   const Vec2f& mousePosition
    - Returns:  bool -- true when the handle took the click
    - Desc:     Starts a drag, or restores the target's original size on a
                double click. The starting size is read from the target's
                resolved bounds plus its outer padding, since padding is inside
                the slot the parent gave it and a drag should move the slot edge
                rather than the drawn edge. Declines when there is no target,
                which is the case for a handle with no visible neighbour on the
                side it points at.
*/
bool Resizer::checkLeftClick(const Vec2f& mousePosition) {
    if (!m_bounds.contains(mousePosition) || !m_uiloRef || !m_target) return false;

    /* 0 means "not clicked yet", so it must not count as a previous click:
       without that guard the first click within the double-click window of
       startup reads as a double click and snaps the target to its original
       size instead of beginning a drag. */
    const uint64_t nowMs = SDL_GetTicks();
    constexpr uint64_t doubleClickMs = 350;
    const bool isDoubleClick = m_lastClickMs != 0
                            && (nowMs - m_lastClickMs) <= doubleClickMs;
    m_lastClickMs = nowMs;

    if (isDoubleClick && m_haveOriginalSize) {
        const ResizerDir dir = m_options.getDirection();
        const bool isHoriz   = (dir == ResizerDir::Left || dir == ResizerDir::Right);
        if (isHoriz) m_target->getModifier().setWidth(m_originalWidth);
        else         m_target->getModifier().setHeight(m_originalHeight);
        m_dragging = false;
        m_uiloRef->setCurrInteractible(this);
        return true;
    }

    const float scale = m_uiloRef->getScale();
    const float padU  = m_target->getOuterPadding();
    m_dragStartW      = m_target->getBounds().size.x / scale + 2.f * padU;
    m_dragStartH      = m_target->getBounds().size.y / scale + 2.f * padU;
    m_dragStart       = mousePosition;
    m_dragging        = true;
    m_uiloRef->setCurrInteractible(this);
    return true;
}

/*
    onDeactivate():
    - Params:   none
    - Returns:  void
    - Desc:     Ends any drag in progress when focus moves elsewhere, so a
                handle cannot be left latched after a click lands on something
                else.
*/
void Resizer::onDeactivate() {
    m_dragging = false;
}

} // namespace uilo
