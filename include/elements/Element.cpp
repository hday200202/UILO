#include "Element.hpp"
#include "../UILO.hpp"

namespace uilo {

    Bounds Element::getBounds() const { return m_bounds; }

    Modifier &Element::getModifier() { return m_modifier; }

    bool Element::isDirty() const { return m_dirty; }

    void Element::resize(const Bounds& parent) {
        
    }

    void Element::setUilo(UILO& uiloRef) { m_uiloRef = &uiloRef; }
}