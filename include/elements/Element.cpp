#include "Element.hpp"
#include "../UILO.hpp"

namespace uilo {

Bounds Element::getBounds() const { return m_bounds; }
Modifier& Element::getModifier() { return m_modifier; }
bool Element::isDirty() const { return m_dirty; }
void Element::erase() { m_markedForDeletion = true; }
ElementType Element::getType() const { return m_type; }

void Element::resize(const Bounds& parent) {
    float p = m_modifier.getPadding();

    // Resolve size against full parent, then shrink by padding on both sides
    m_bounds.size.x = m_modifier.getWidth().resolve(parent.size.x) - 2.f * p;
    m_bounds.size.y = m_modifier.getHeight().resolve(parent.size.y) - 2.f * p;

    // Padded inner area used for position alignment
    Bounds inner = {
        {parent.position.x + p, parent.position.y + p},
        {parent.size.x - 2.f * p, parent.size.y - 2.f * p}
    };

    Align align = m_modifier.getAlign();

    // Horizontal axis
    if (hasAlign(align, Align::LEFT))
        m_bounds.position.x = inner.position.x;
    else if (hasAlign(align, Align::RIGHT))
        m_bounds.position.x = inner.right() - m_bounds.size.x;
    else if (hasAlign(align, Align::CENTER_X))
        m_bounds.position.x = inner.position.x + (inner.size.x - m_bounds.size.x) * 0.5f;
    else
        m_bounds.position.x = inner.position.x;

    // Vertical axis
    if (hasAlign(align, Align::TOP))
        m_bounds.position.y = inner.position.y;
    else if (hasAlign(align, Align::BOTTOM))
        m_bounds.position.y = inner.bottom() - m_bounds.size.y;
    else if (hasAlign(align, Align::CENTER_Y))
        m_bounds.position.y = inner.position.y + (inner.size.y - m_bounds.size.y) * 0.5f;
    else
        m_bounds.position.y = inner.position.y;
}

bool Element::checkRightClick(const Vec2f& mousePosition) {
    if (!m_bounds.intersects({mousePosition, {1, 1}})) return false;
    if (m_modifier.getOnRightClick()) m_modifier.getOnRightClick()();
    return true;
}

bool Element::checkLeftClick(const Vec2f& mousePosition) {
    if (!m_bounds.intersects({mousePosition, {1, 1}})) return false;
    if (m_modifier.getOnLeftClick()) m_modifier.getOnLeftClick()();
    return true;
}

bool Element::checkHover(const Vec2f& mousePosition) {
    if (!m_bounds.intersects({mousePosition, {1, 1}})) {
        m_hovered = false;
        return false;
    }
    if (!m_hovered) {
        m_hovered = true;
        if (m_modifier.getOnHover()) m_modifier.getOnHover()();
    }
    return true;
}

bool Element::checkScroll(const Vec2f& mousePosition, float delta) {
    if (!m_bounds.intersects({mousePosition, {1, 1}})) return false;
    if (m_modifier.getOnScroll()) m_modifier.getOnScroll()(delta);
    return true;
}

void Element::setUilo(UILO& uiloRef) {
    m_uiloRef = &uiloRef;
    uiloRef.m_elementPool.emplace_back(this);
    if (!m_name.empty())
        uiloRef.m_elements[m_name] = this;
}

} // namespace uilo