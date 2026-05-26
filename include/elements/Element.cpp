#include "Element.hpp"
#include "../UILO.hpp"

namespace uilo {
    
    sf::FloatRect Element::getBounds() const { return m_bounds; }
    Modifier& Element::getModifier() { return m_modifier; }
    bool Element::isDirty() const { return m_dirty; }
    void Element::erase() { m_markedForDeletion = true; }
    ElementType Element::getType() const { return m_type; }

    void Element::resize(const sf::FloatRect& parent) {
        float scale = m_uiloRef ? m_uiloRef->getScale() : 1.f;
        float op = m_modifier.getOuterPadding() * scale;

        auto resolveScaled = [&](Dimension dim, float parentSize) -> float {
            return dim.percent ? (dim.value / 100.f * parentSize) : (dim.value * scale);
        };

        // Resolve against parent, shrink by outer padding on lrtb
        const sf::Vector2f oldSize = m_bounds.size;
        m_bounds.size.x = resolveScaled(m_modifier.getWidth(),  parent.size.x) - (2.f * op);
        m_bounds.size.y = resolveScaled(m_modifier.getHeight(), parent.size.y) - (2.f * op);
        if (m_bounds.size != oldSize) m_dirty = true;

        sf::FloatRect inner = {
            {parent.position.x + op, parent.position.y + op},
            {parent.size.x - (2.f * op), parent.size.y - (2.f * op)}
        };

        Align align = m_modifier.getAlign();

        // Horizontal alignment
        if (hasAlign(align, Align::Left))
            m_bounds.position.x = inner.position.x;
        else if (hasAlign(align, Align::Right))
            m_bounds.position.x = inner.position.x + inner.size.x - m_bounds.size.x;
        else if (hasAlign(align, Align::CenterX))
            m_bounds.position.x = inner.position.x + (inner.size.x - m_bounds.size.x) * 0.5f;
        else m_bounds.position.x = inner.position.x;

        // Vertical alignment
        if (hasAlign(align, Align::Top))
            m_bounds.position.y = inner.position.y;
        else if (hasAlign(align, Align::Bottom))
            m_bounds.position.y = inner.position.y + inner.size.y - m_bounds.size.y;
        else if (hasAlign(align, Align::CenterY))
            m_bounds.position.y = inner.position.y + (inner.size.y - m_bounds.size.y) * 0.5f;
        else m_bounds.position.y = inner.position.y;
    }

    bool Element::checkLeftClick(const sf::Vector2f& mousePosition) {
        if (!m_bounds.contains(mousePosition)) return false;
        if (m_modifier.getOnLeftClick()) m_modifier.getOnLeftClick()();
        return true;
    }

    bool Element::checkRightClick(const sf::Vector2f& mousePosition) {
        if (!m_bounds.contains(mousePosition)) return false;
        if (m_modifier.getOnRightClick()) m_modifier.getOnRightClick()();
        return true;
    }

    bool Element::checkHover(const sf::Vector2f& mousePosition) {
        if (!m_bounds.contains(mousePosition)) {
            if (m_hovered) { m_hovered = false; m_dirty = true; }
            return false;
        }
        if (!m_hovered) {
            m_hovered = true;
            m_dirty = true;
            if (m_modifier.getOnHover()) m_modifier.getOnHover()();
        }
        if (m_uiloRef && m_modifier.getOnLeftClick())
            m_uiloRef->requestCursor(sf::Cursor::Type::Hand, 1);
        return true;
    }

    bool Element::checkScroll(const sf::Vector2f& mousePosition, float delta) {
        if (!m_bounds.contains(mousePosition)) return false;
        if (m_modifier.getOnScroll()) {
            m_modifier.getOnScroll()(delta);
            return true;
        }
        return false;
    }

    void Element::setUILO(UILO& uiloRef) {
        m_uiloRef = &uiloRef;
        uiloRef.m_elementPool.emplace_back(this);
        if (!m_name.empty()) uiloRef.m_elements[m_name] = this;
    }

}