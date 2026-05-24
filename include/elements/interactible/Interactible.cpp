#include "Interactible.hpp"
#include "../../UILO.hpp"

namespace uilo {

bool Interactible::checkLeftClick(const sf::Vector2f& mousePosition) {
    if (!m_bounds.contains(mousePosition)) return false;
    m_uiloRef->setCurrInteractible(this);
    if (m_modifier.getOnLeftClick()) m_modifier.getOnLeftClick()();
    return true;
}

bool Interactible::checkRightClick(const sf::Vector2f& mousePosition) {
    if (!m_bounds.contains(mousePosition)) return false;
    m_uiloRef->setCurrInteractible(this);
    if (m_modifier.getOnRightClick()) m_modifier.getOnRightClick()();
    return true;
}

}
