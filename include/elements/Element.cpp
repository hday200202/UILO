#include "Element.hpp"
#include "../UILO.hpp"

namespace uilo {

    Bounds Element::getBounds() const { return m_bounds; }

    Modifier &Element::getModifier() { return m_modifier; }

    bool Element::isDirty() const { return m_dirty; }

    void Element::resize(const Bounds& parent) {
        // Resolve size from modifier dimensions against parent size
        m_bounds.size.x = m_modifier.getWidth().resolve(parent.size.x);
        m_bounds.size.y = m_modifier.getHeight().resolve(parent.size.y);

        // Resolve position based on alignment
        Align align = m_modifier.getAlign();

        // Horizontal axis
        if (hasAlign(align, Align::LEFT))
            m_bounds.position.x = parent.position.x;
        else if (hasAlign(align, Align::RIGHT))
            m_bounds.position.x = parent.right() - m_bounds.size.x;
        else if (hasAlign(align, Align::CENTER_X))
            m_bounds.position.x = parent.position.x + (parent.size.x - m_bounds.size.x) * 0.5f;
        else
            m_bounds.position.x = parent.position.x;

        // Vertical axis
        if (hasAlign(align, Align::TOP))
            m_bounds.position.y = parent.position.y;
        else if (hasAlign(align, Align::BOTTOM))
            m_bounds.position.y = parent.bottom() - m_bounds.size.y;
        else if (hasAlign(align, Align::CENTER_Y))
            m_bounds.position.y = parent.position.y + (parent.size.y - m_bounds.size.y) * 0.5f;
        else
            m_bounds.position.y = parent.position.y;
    }

    ElementType Element::getType() const { return m_type; }

    void Element::setUilo(UILO& uiloRef) {
        m_uiloRef = &uiloRef;
        if (!m_name.empty())
            uiloRef.m_elements[m_name] = this;
    }
}