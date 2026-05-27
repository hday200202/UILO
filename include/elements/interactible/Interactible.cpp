#include "Interactible.hpp"
#include "../../UILO.hpp"

namespace uilo {

bool Interactible::checkLeftClick(const Vec2f& mousePosition) {
    if (!m_bounds.contains(mousePosition)) return false;
    m_uiloRef->setCurrInteractible(this);
    if (m_modifier.getOnLeftClick()) m_modifier.getOnLeftClick()(this);
    return true;
}

bool Interactible::checkRightClick(const Vec2f& mousePosition) {
    if (!m_bounds.contains(mousePosition)) return false;
    m_uiloRef->setCurrInteractible(this);
    if (m_modifier.getOnRightClick()) m_modifier.getOnRightClick()(this);
    return true;
}

}
