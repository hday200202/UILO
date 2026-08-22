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
    floatingAt(const Vec2f& mousePosition):
    - Params:   const Vec2f& mousePosition
    - Returns:  Element* -- the floating child that owns the pointer, or nullptr
    - Desc:     The topmost visible floating child containing the position.
                Whoever finds one stops there: a floating child is a surface laid
                over its siblings, so it takes the event whether or not anything
                in it claims pointer events. Were it offered the event and then
                allowed to decline, a panel of plain labels would let the button
                underneath light up through it. Searched in reverse because the
                last child is drawn last, and an invisible one is not there at
                all.
*/
Element* Container::floatingAt(const Vec2f& mousePosition) const {
    for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
        Element* child = *it;
        const Modifier& mod = child->getModifier();
        if (!mod.isFloating() || !mod.getVisible()) continue;
        if (child->getBounds().contains(mousePosition)) return child;
    }
    return nullptr;
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
    - A floating child under the pointer short-circuits all of that: it is the
      only thing offered the click, and the event is reported consumed however it
      answers.
*/
bool Container::checkLeftClick(const Vec2f& mousePosition) {
    /* A press on a draggable floating child starts a drag rather than being
       treated as an ordinary click on it. */
    if (beginFloatingDrag(mousePosition)) return true;

    if (Element* top = floatingAt(mousePosition)) {
        top->checkLeftClick(mousePosition);
        return true;
    }

    bool childClicked = false;
    for (auto& child : m_children) {
        if (childClicked) break;
        if (child->getType() == ElementType::Resizer) continue;
        if (child->getModifier().isFloating()) continue;
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
    if (Element* top = floatingAt(mousePosition)) {
        top->checkRightClick(mousePosition);
        return true;
    }

    bool childClicked = false;
    for (auto& child : m_children) {
        if (childClicked) break;
        if (child->getModifier().isFloating()) continue;
        if (child->getBounds().contains(mousePosition))
            childClicked |= child->checkRightClick(mousePosition);
    }

    if (!childClicked && m_bounds.contains(mousePosition)) {
        if (m_modifier.getOnRightClick()) m_modifier.getOnRightClick()(this);
        if (openContextMenu(mousePosition)) return true;
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
    - When a floating child owns the pointer, every other child is told the cursor
      is nowhere and the container does not claim the hover either: it is behind
      the panel, so its own handlers have no business firing.
*/
bool Container::checkHover(const Vec2f& mousePosition) {
    static constexpr Vec2f kNowhere{-1e9f, -1e9f};

    Element* top = floatingAt(mousePosition);
    bool childHovered = false;

    for (auto& child : m_children) {
        if (child->getType() == ElementType::Resizer) continue;
        /* Only the owner is told where the pointer is; the rest have to hear that
           it is nowhere so their hover clears. A floating child that is not the
           owner is blind either way -- otherwise a hidden one, whose stale bounds
           still cover the cursor, would light up. */
        const bool owns  = child == top;
        const bool blind = !owns && (top || child->getModifier().isFloating());
        if (child->checkHover(blind ? kNowhere : mousePosition))
            childHovered = true;
    }

    const bool inside = !childHovered && !top && m_bounds.contains(mousePosition);

    if (inside && !m_hovered) {
        m_hovered = true; m_dirty = true;
        if (m_modifier.getOnHoverEnter()) m_modifier.getOnHoverEnter()(this);
    } else if (!inside && m_hovered) {
        m_hovered = false; m_dirty = true;
        if (m_modifier.getOnHoverExit()) m_modifier.getOnHoverExit()(this);
    }

    if (inside && m_uiloRef && m_modifier.getOnLeftClick())
        m_uiloRef->requestCursor(CursorType::Hand, 1);
    return childHovered || top != nullptr || (inside && claimsPointerEvents());
}


/*
    checkScroll(const Vec2f& mousePosition, float delta, bool precise, bool momentum):
    - Params:   const Vec2f& mousePosition, float delta, bool precise, bool
                momentum
    - Returns:  bool -- true when the event was consumed
    - Desc:     Offers the scroll to the children under the cursor, stopping at
                the first that takes it, and otherwise falls back to the
                Modifier's onScroll handler. A plain Container does not scroll
                itself -- Row and Column override this.
    - A floating child under the pointer swallows the event even when it scrolls
      nothing, or the view behind it would slide out from under the cursor.
*/
bool Container::checkScroll(
    const Vec2f& mousePosition,
    float delta,
    bool precise,
    bool momentum
) {
    if (Element* top = floatingAt(mousePosition)) {
        top->checkScroll(mousePosition, delta, precise, momentum);
        return true;
    }

    for (auto& child : m_children) {
        if (child->getModifier().isFloating()) continue;
        if (child->getBounds().contains(mousePosition))
            if (child->checkScroll(mousePosition, delta, precise, momentum)) return true;
    }

    if (m_bounds.contains(mousePosition) && m_modifier.getOnScroll()) {
        m_modifier.getOnScroll()(this, delta);
        return true;
    }

    return false;
}


/*
    checkScroll(const Vec2f& mousePosition, Vec2f delta, bool precise, bool momentum):
    - Params:   const Vec2f& mousePosition, Vec2f delta, bool precise, bool
                momentum
    - Returns:  bool -- true when the event was consumed
    - Desc:     The two-axis form of the above, passing the whole delta down.
*/
bool Container::checkScroll(
    const Vec2f& mousePosition,
    Vec2f delta,
    bool precise,
    bool momentum
) {
    if (Element* top = floatingAt(mousePosition)) {
        top->checkScroll(mousePosition, delta, precise, momentum);
        return true;
    }

    for (auto& child : m_children) {
        if (child->getModifier().isFloating()) continue;
        if (child->getBounds().contains(mousePosition))
            if (child->checkScroll(mousePosition, delta, precise, momentum)) return true;
    }

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
                A floating child under the pointer takes the gesture alone, which
                is also what stops Row and Column from zooming themselves under
                one.
*/
bool Container::checkZoom(const Vec2f& mousePosition, float magnification) {
    if (Element* top = floatingAt(mousePosition)) {
        top->checkZoom(mousePosition, magnification);
        return true;
    }

    bool consumed = false;
    for (auto& child : m_children) {
        if (child->getModifier().isFloating()) continue;
        if (child->getBounds().contains(mousePosition))
            if (child->checkZoom(mousePosition, magnification)) consumed = true;
    }
    return consumed;
}



/*
    tickFloating(float dt):
    - Params:   float dt
    - Returns:  void
    - Desc:     Places and updates the container's floating children. Their slot
                is the content area rather than a share of the flow, so a free
                position of zero puts one in the container's top-left corner,
                inside its inner padding. A drag in progress is applied first, so
                the child lands at its new position in the same frame it moved
                rather than one frame behind the pointer.
*/
void Container::tickFloating(float dt) {
    const Rectf area = contentArea();

    if (m_dragChild) {
        float mx = 0.f, my = 0.f;
        const uint32_t buttons = SDL_GetMouseState(&mx, &my);
        if (!(buttons & SDL_BUTTON_MASK(SDL_BUTTON_LEFT)) || !m_uiloRef) {
            m_dragChild = nullptr;
        } else {
            /* Free position is stored unscaled, so undo the scale on the way in
               or a drag would run away from the pointer on a HiDPI display. */
            const Vec2f mouse = m_uiloRef->getMousePosition();
            const float sc    = m_uiloRef->getScale() > 0.f ? m_uiloRef->getScale() : 1.f;
            m_dragChild->getModifier().setFreePosition(
                Dimension{(mouse.x - m_dragOffset.x - area.position.x) / sc, false},
                Dimension{(mouse.y - m_dragOffset.y - area.position.y) / sc, false});
            m_uiloRef->requestCursor(CursorType::Crosshair, 10);
        }
    }

    for (auto* child : m_children) {
        if (!child->getModifier().isFloating()) continue;
        Rectf slot = area;
        child->tick(slot, dt);
    }
}


/*
    renderFloating(float rounding):
    - Params:   float rounding
    - Returns:  void
    - Desc:     Draws the floating children above everything else the container
                holds, clipped to the container so a floating panel cannot spill
                out of the pane it belongs to. `rounding` comes from the
                container's own Options, which Container cannot see.
*/
void Container::renderFloating(float rounding) {
    if (!m_uiloRef) return;

    const float rr = rounding * m_uiloRef->getScale();
    for (auto* child : m_children) {
        if (!child->getModifier().isFloating()) continue;
        m_uiloRef->getRenderer().pushRoundClip(m_bounds, rr);
        child->render();
        m_uiloRef->getRenderer().popRoundClip();
    }
}


/*
    beginFloatingDrag(const Vec2f& mousePosition):
    - Params:   const Vec2f& mousePosition
    - Returns:  bool -- true when a drag was started
    - Desc:     Starts dragging the topmost draggable floating child under the
                pointer. The grab offset is kept so the element does not jump to
                centre itself on the cursor.
*/
bool Container::beginFloatingDrag(const Vec2f& mousePosition) {
    for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
        Element* child = *it;
        const Modifier& mod = child->getModifier();
        if (!mod.isFloating() || !mod.isDraggable() || !mod.getVisible()) continue;
        if (!child->getBounds().contains(mousePosition)) continue;

        m_dragChild  = child;
        m_dragOffset = { mousePosition.x - child->getBounds().position.x,
                         mousePosition.y - child->getBounds().position.y };
        return true;
    }
    return false;
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