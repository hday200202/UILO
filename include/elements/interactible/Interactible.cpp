#include "Interactible.hpp"
#include "../../UILO.hpp"

namespace uilo {

/*
    checkLeftClick(const Vec2f& mousePosition):
    - Params:   const Vec2f& mousePosition
    - Returns:  bool -- true when the click landed on this element
    - Desc:     Takes focus and fires the Modifier's left-click handler.
                Claiming focus is what deactivates whichever interactible held
                it before, so an open dropdown closes when a textbox is clicked.
                The click is always consumed on a hit, handler or not.
*/
bool Interactible::checkLeftClick(const Vec2f& mousePosition) {
    if (!m_bounds.contains(mousePosition)) return false;
    m_uiloRef->setCurrInteractible(this);
    if (m_modifier.getOnLeftClick()) m_modifier.getOnLeftClick()(this);
    return true;
}


/*
    checkRightClick(const Vec2f& mousePosition):
    - Params:   const Vec2f& mousePosition
    - Returns:  bool -- true when the click landed on this element
    - Desc:     As checkLeftClick, for the right button.
*/
bool Interactible::checkRightClick(const Vec2f& mousePosition) {
    if (!m_bounds.contains(mousePosition)) return false;
    m_uiloRef->setCurrInteractible(this);
    if (m_modifier.getOnRightClick()) m_modifier.getOnRightClick()(this);
    return true;
}

}
