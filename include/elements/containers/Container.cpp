#include "Container.hpp"

#include <algorithm>
#include "../../UILO.hpp"

namespace uilo {

/*
    Container(Modifier modifier, contains children, const std::string& name):
    - Params:   Modifier modifier, contains children, const std::string& name
    - Returns:  Container
    - Desc:     Constructs a container from a modifier and an initializer list
                of children, reserving room for them up front.
*/
Container::Container(
    Modifier modifier,
    contains children,
    const std::string& name
) {
    m_modifier = modifier;
    m_name = name;
    m_children.reserve(children.size());

    for (auto& child : children) m_children.push_back(child);
}


/*
    isDirty():
    - Params:   none
    - Returns:  bool
    - Desc:     True when this container or anything below it needs redrawing,
                so a change deep in the tree still reaches the top.
*/
bool Container::isDirty() const {
    if (m_dirty) return true;
    for (auto* child : m_children) {
        if (child->isDirty()) return true;
    }
    return false;
}


/*
    checkLeftClick(const Vec2f& mousePosition):
    - Params:   const Vec2f& mousePosition
    - Returns:  bool -- true when the click was claimed here or below
    - Desc:     Offers the click to every child under the cursor first, and only
                fires the container's own handler when none of them took it.
                That ordering is what lets a decorative child -- a Text label,
                an indent Spacer -- sit inside a clickable row without
                swallowing the press. Resizers are skipped; they are hit-tested
                separately because they sit outside the layout flow.
*/
bool Container::checkLeftClick(const Vec2f& mousePosition) {
    bool childClicked = false;

    for (auto& child : m_children) {
        if (child->getType() == ElementType::Resizer) continue;
        if (child->getBounds().contains(mousePosition))
            childClicked |= child->checkLeftClick(mousePosition);
    }

    if (!childClicked && m_bounds.contains(mousePosition)) {
        if (m_modifier.getOnLeftClick()) m_modifier.getOnLeftClick()(this);
        return claimsPointerEvents();
    }

    return childClicked;
}


/*
    checkRightClick(const Vec2f& mousePosition):
    - Params:   const Vec2f& mousePosition
    - Returns:  bool -- true when the click was claimed here or below
    - Desc:     As checkLeftClick, for the right button.
*/
bool Container::checkRightClick(const Vec2f& mousePosition) {
    bool childClicked = false;

    for (auto& child : m_children)
        if (child->getBounds().contains(mousePosition))
            childClicked |= child->checkRightClick(mousePosition);

    if (!childClicked && m_bounds.contains(mousePosition)) {
        if (m_modifier.getOnRightClick()) m_modifier.getOnRightClick()(this);
        return claimsPointerEvents();
    }

    return childClicked;
}


/*
    checkHover(const Vec2f& mousePosition):
    - Params:   const Vec2f& mousePosition
    - Returns:  bool -- true when anything in this subtree took the hover
    - Desc:     Recurses into every child unconditionally, not just the ones
                under the cursor, so a child that has just been left still fires
                its own onHoverExit instead of staying stuck hovered. The
                container claims the hover itself only when no child did and the
                cursor is inside it, and requests the hand cursor when it has a
                click handler of its own.
*/
bool Container::checkHover(const Vec2f& mousePosition) {
    bool childHovered = false;

    for (auto& child : m_children) {
        if (child->getType() == ElementType::Resizer) continue;
        if (child->checkHover(mousePosition)) childHovered = true;
    }

    const bool inside = !childHovered && m_bounds.contains(mousePosition);

    if (inside && !m_hovered) {
        m_hovered = true; m_dirty = true;
        if (m_modifier.getOnHoverEnter()) m_modifier.getOnHoverEnter()(this);
    } else if (!inside && m_hovered) {
        m_hovered = false; m_dirty = true;
        if (m_modifier.getOnHoverExit()) m_modifier.getOnHoverExit()(this);
    }

    if (inside && m_uiloRef && m_modifier.getOnLeftClick())
        m_uiloRef->requestCursor(CursorType::Hand, 1);
    return childHovered || (inside && claimsPointerEvents());
}


/*
    checkScroll(const Vec2f& mousePosition, float delta, bool precise, bool momentum):
    - Params:   const Vec2f& mousePosition, float delta, bool precise,
                bool momentum
    - Returns:  bool -- true when the event was consumed
    - Desc:     Offers the scroll to the children under the cursor, stopping at
                the first that takes it, and otherwise falls back to the
                Modifier's onScroll handler. A plain Container does not scroll
                itself -- Row and Column override this.
*/
bool Container::checkScroll(
    const Vec2f& mousePosition,
    float delta,
    bool precise,
    bool momentum
) {
    for (auto& child : m_children)
        if (child->getBounds().contains(mousePosition))
            if (child->checkScroll(mousePosition, delta, precise, momentum)) return true;

    if (m_bounds.contains(mousePosition) && m_modifier.getOnScroll()) {
        m_modifier.getOnScroll()(this, delta);
        return true;
    }

    return false;
}


/*
    checkScroll(const Vec2f& mousePosition, Vec2f delta, bool precise, bool momentum):
    - Params:   const Vec2f& mousePosition, Vec2f delta, bool precise,
                bool momentum
    - Returns:  bool -- true when the event was consumed
    - Desc:     The two-axis form of the above, passing the whole delta down.
*/
bool Container::checkScroll(
    const Vec2f& mousePosition,
    Vec2f delta,
    bool precise,
    bool momentum
) {
    for (auto& child : m_children)
        if (child->getBounds().contains(mousePosition))
            if (child->checkScroll(mousePosition, delta, precise, momentum)) return true;

    if (m_bounds.contains(mousePosition) && m_modifier.getOnScroll()) {
        m_modifier.getOnScroll()(this, delta.y);
        return true;
    }

    return false;
}


/*
    checkZoom(const Vec2f& mousePosition, float magnification):
    - Params:   const Vec2f& mousePosition, float magnification
    - Returns:  bool -- true when the gesture was consumed
    - Desc:     Offers the zoom to every child under the cursor without stopping
                at the first taker, so the deepest or last-drawn one wins. A
                Canvas drawn over its siblings needs that to claim the gesture.
*/
bool Container::checkZoom(const Vec2f& mousePosition, float magnification) {
    bool consumed = false;
    for (auto& child : m_children)
        if (child->getBounds().contains(mousePosition))
            if (child->checkZoom(mousePosition, magnification)) consumed = true;
    return consumed;
}


/*
    addElement(Element* element):
    - Params:   Element* element
    - Returns:  void
    - Desc:     Appends a child. When the container is already bound to a UILO
                the new child is bound too, so an element added at runtime lands
                in the element pool and can resolve palette roles like any
                other.
*/
void Container::addElement(Element* element) {
    if (!element) return;
    m_children.push_back(element);
    if (m_uiloRef) {
        element->setUILO(*m_uiloRef);
    }
    m_dirty = true;
}


/*
    pruneChildren():
    - Params:   none
    - Returns:  void
    - Desc:     Drops children that have been erased. Called at the top of
                layout rather than at erase() time, because a click handler is
                free to erase an element while the parent is still walking its
                child list.
*/
void Container::pruneChildren() {
    m_children.erase(
        std::remove_if(
            m_children.begin(), m_children.end(), 
            [](Element* e) { return e->m_markedForDeletion; }
        ), m_children.end()
    );
}


/*
    setUILO(UILO& uiloRef):
    - Params:   UILO& uiloRef
    - Returns:  void
    - Desc:     Binds this container and, recursively, every child to a UILO.
                This is what puts each element in the pool and gives it access
                to the palette and scale.
*/
void Container::setUILO(UILO& uiloRef) {
    Element::setUILO(uiloRef);
    for (auto& child : m_children)
        child->setUILO(uiloRef);
}


/*
    collectResizers(std::vector<Element*>& out):
    - Params:   std::vector<Element*>& out
    - Returns:  void
    - Desc:     Gathers every Resizer in the subtree. UILO hit-tests them ahead
                of the tree walk, so it needs them in one flat list.
*/
void Container::collectResizers(std::vector<Element*>& out) {
    for (auto* child : m_children) {
        if (child->getType() == ElementType::Resizer)
            out.push_back(child);
        else
            child->collectResizers(out);
    }
}

}