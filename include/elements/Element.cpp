#include "Element.hpp"

#include <algorithm>
#include "../UILO.hpp"

namespace uilo {

/*
    getBounds():
    - Params:   none
    - Returns:  Rectf
    - Desc:     The element's resolved position and size in render pixels, as of
                the last layout pass.
*/
Rectf Element::getBounds() const { return m_bounds; }


/*
    getModifier():
    - Params:   none
    - Returns:  Modifier&
    - Desc:     The element's live modifier. Mutable, so a handler can change
                size, alignment, visibility or another callback at runtime.
*/
Modifier& Element::getModifier() { return m_modifier; }


/*
    isDirty():
    - Params:   none
    - Returns:  bool
    - Desc:     Whether the element needs redrawing. Containers override this to
                also report a dirty descendant.
*/
bool Element::isDirty() const { return m_dirty; }


/*
    erase():
    - Params:   none
    - Returns:  void
    - Desc:     Marks the element for deletion rather than deleting it. UILO
                sweeps marked elements out of the pool between frames, so this
                is safe to call from a handler while the tree is being walked.
*/
void Element::erase() { m_markedForDeletion = true; }


/*
    getType():
    - Params:   none
    - Returns:  ElementType
    - Desc:     Which kind of element this is, for the few places that branch on
                type instead of on a virtual call.
*/
ElementType Element::getType() const { return m_type; }


/*
    getDeltaTime():
    - Params:   none
    - Returns:  float
    - Desc:     Seconds since the previous frame, read from the owning UILO. 0
                when the element is not bound to one, so per-frame animation in
                a handler degrades to standing still rather than jumping.
*/
float Element::getDeltaTime() const { return m_uiloRef ? m_uiloRef->getDeltaTime() : 0.f; }


/*
    resolveColor(std::string_view role, Color literal):
    - Params:   std::string_view role, Color literal
    - Returns:  Color
    - Desc:     Resolves a color through the owning UILO's Palette. Returns
                `literal` unchanged when there is no UILO, or when the role is
                empty, "none", or names nothing the palette knows -- so a role
                is always safe to set and a palette is never required.
*/
Color Element::resolveColor(std::string_view role, Color literal) const {
    if (!m_uiloRef) return literal;
    return m_uiloRef->getPalette().resolve(role, literal);
}


/*
    resolveGradient(...):
    - Params:   const Gradient& literal, std::string_view gradientRole, Color
                out[4]
    - Returns:  bool -- true when the result is worth drawing
    - Desc:     Picks the gradient to draw and resolves its stops to colors. A
                non-empty role naming a palette gradient wins over the literal
                one, and each stop's own role is then resolved. Fills `out` in
                TL, TR, BL, BR order, and reports false for an inactive gradient
                or one that came out fully transparent so the caller can fall
                back to a flat fill.
*/
bool Element::resolveGradient(
    const Gradient& literal,
    std::string_view gradientRole,
    Color out[4]
) const {
    if (!m_uiloRef) return false;
    const Palette& palette = m_uiloRef->getPalette();

    const Gradient* g = &literal;
    if (!gradientRole.empty()) {
        if (const Gradient* named = palette.getGradient(gradientRole))
            g = named;
    }
    if (!g->active()) return false;

    g->resolve(palette, out);
    return out[0].a > 0 || out[1].a > 0 || out[2].a > 0 || out[3].a > 0;
}


/*
    tick(Rectf& parentBounds, float dt):
    - Params:   Rectf& parentBounds, float dt
    - Returns:  void
    - Desc:     Non-virtual wrapper that fires onUpdateStart and onUpdateEnd
                around the virtual update(). Everything that walks the tree --
                containers, Page, a dropdown popup -- calls this rather than
                update() directly, so the lifecycle hooks always run.
*/
void Element::tick(Rectf& parentBounds, float dt) {
    if (m_uiloRef && m_uiloRef->isForcingTreeUpdate()) {
        m_dirty = true;
    }

    if (m_modifier.getOnUpdateStart()) m_modifier.getOnUpdateStart()(this);
    update(parentBounds, dt);
    if (m_modifier.getOnUpdateEnd())   m_modifier.getOnUpdateEnd()(this);
}


/*
    resize(const Rectf& parent):
    - Params:   const Rectf& parent
    - Returns:  void
    - Desc:     Works out the element's own bounds inside the slot its parent
                gave it. A percent dimension is taken against the slot; a pixel
                dimension is multiplied by UILO's scale. Outer padding shrinks
                the element on all four sides and insets the box it is aligned
                within, so padding never moves a sibling. The element is then
                placed against that inset box according to its alignment flags,
                defaulting to the top-left corner.
    - A floating element skips all of that after sizing: it is placed at its free
      position relative to the slot's corner, since it is outside the flow and
      has neither siblings to be spaced from nor an alignment box to sit in.
*/
/*
    contentArea():
    - Params:   none
    - Returns:  Rectf
    - Desc:     Where this element's content belongs: its bounds inset on all
                four sides by the inner padding its Options reports. The element
                still draws its own background and is hit-tested against the
                full bounds, so inner padding moves the content and nothing
                else.
*/
Rectf Element::contentArea() const {
    const float scale = m_uiloRef ? m_uiloRef->getScale() : 1.f;
    const float ip    = getInnerPadding() * scale;
    if (ip <= 0.f) return m_bounds;

    return Rectf{
        { m_bounds.position.x + ip, m_bounds.position.y + ip },
        { std::max(0.f, m_bounds.size.x - 2.f * ip),
          std::max(0.f, m_bounds.size.y - 2.f * ip) }
    };
}


void Element::resize(const Rectf& parent) {
    float scale = m_uiloRef ? m_uiloRef->getScale() : 1.f;
    float op = getOuterPadding() * scale;

    /* A _flex size fills whatever it is given: the container works out the share
       and hands it over as the slot, so by the time it reaches here there is
       nothing left to divide. */
    auto resolveScaled = [&](Dimension dim, float parentSize) -> float {
        if (dim.flex)    return parentSize;
        if (dim.percent) return dim.value / 100.f * parentSize;
        return dim.value * scale;
    };

    const Vec2f oldSize = m_bounds.size;

    /* A floating element is outside the flow: its size still resolves against
       the container, but outer padding means nothing (it has no siblings to be
       spaced from) and alignment means nothing (its position is given). */
    if (m_modifier.isFloating()) {
        m_bounds.size.x = resolveScaled(m_modifier.getWidth(),  parent.size.x);
        m_bounds.size.y = resolveScaled(m_modifier.getHeight(), parent.size.y);
        if (m_bounds.size != oldSize) m_dirty = true;

        m_bounds.position = {
            parent.position.x + resolveScaled(m_modifier.getFreeX(), parent.size.x),
            parent.position.y + resolveScaled(m_modifier.getFreeY(), parent.size.y),
        };
        return;
    }

    m_bounds.size.x = resolveScaled(m_modifier.getWidth(),  parent.size.x) - (2.f * op);
    m_bounds.size.y = resolveScaled(m_modifier.getHeight(), parent.size.y) - (2.f * op);
    if (m_bounds.size != oldSize) m_dirty = true;

    Rectf inner = {
        {parent.position.x + op, parent.position.y + op},
        {parent.size.x - (2.f * op), parent.size.y - (2.f * op)}
    };

    Align align = m_modifier.getAlign();

    /* Horizontal placement against the inset box. */
    if (hasAlign(align, Align::Left))
        m_bounds.position.x = inner.position.x;
    else if (hasAlign(align, Align::Right))
        m_bounds.position.x = inner.position.x + inner.size.x - m_bounds.size.x;
    else if (hasAlign(align, Align::CenterX))
        m_bounds.position.x = inner.position.x + (inner.size.x - m_bounds.size.x) * 0.5f;
    else m_bounds.position.x = inner.position.x;

    /* Vertical placement against the inset box. */
    if (hasAlign(align, Align::Top))
        m_bounds.position.y = inner.position.y;
    else if (hasAlign(align, Align::Bottom))
        m_bounds.position.y = inner.position.y + inner.size.y - m_bounds.size.y;
    else if (hasAlign(align, Align::CenterY))
        m_bounds.position.y = inner.position.y + (inner.size.y - m_bounds.size.y) * 0.5f;
    else m_bounds.position.y = inner.position.y;
}


/*
    claimsPointerEvents():
    - Params:   none
    - Returns:  bool
    - Desc:     Whether this element claims a pointer event that lands on it,
                stopping the search there. An element with nothing attached is
                transparent, so a decorative child -- the Text label of a
                clickable row, a Spacer used for indentation, a background Image
                -- does not swallow the click or hover of the container it sits
                in. Widgets that are interactive by nature override this to
                true, so a press over a Button or Slider never falls through to
                whatever is behind it even when no callback was attached.
*/
bool Element::claimsPointerEvents() const {
    return m_modifier.getOnLeftClick()  || m_modifier.getOnRightClick()
        || m_modifier.getOnHoverEnter() || m_modifier.getOnHoverExit();
}


/*
    checkLeftClick(const Vec2f& mousePosition):
    - Params:   const Vec2f& mousePosition
    - Returns:  bool -- true when the element claimed the click
    - Desc:     Fires the Modifier's left-click handler when the position is
                inside the element, and reports whether the click should stop
                here.
*/
bool Element::checkLeftClick(const Vec2f& mousePosition) {
    if (!m_bounds.contains(mousePosition)) return false;
    if (m_modifier.getOnLeftClick()) m_modifier.getOnLeftClick()(this);
    return claimsPointerEvents();
}


/*
    checkRightClick(const Vec2f& mousePosition):
    - Params:   const Vec2f& mousePosition
    - Returns:  bool -- true when the element claimed the click
    - Desc:     As checkLeftClick, for the right button.
*/
bool Element::checkRightClick(const Vec2f& mousePosition) {
    if (!m_bounds.contains(mousePosition)) return false;
    if (m_modifier.getOnRightClick()) m_modifier.getOnRightClick()(this);
    return claimsPointerEvents();
}


/*
    checkHover(const Vec2f& mousePosition):
    - Params:   const Vec2f& mousePosition
    - Returns:  bool -- true when the element claimed the hover
    - Desc:     Tracks the hovered state, fires onHoverEnter and onHoverExit on
                the transitions, and asks for the element's cursor. The enter
                and exit bookkeeping always runs, but only an element that
                actually wants pointer events masks its parent's hover, so a
                decorative child does not blank out the row it sits in.
    - The cursor is requested on every frame the pointer is inside, not once on
      the way in: UILO clears the request pool at the top of each frame, so a
      one-shot request would be undone immediately. A Modifier cursor wins over
      the hand a clickable element asks for, which is what lets an explicit
      Arrow suppress it.
*/
bool Element::checkHover(const Vec2f& mousePosition) {
    if (!m_bounds.contains(mousePosition)) {
        if (m_hovered) {
            m_hovered = false; m_dirty = true;
            if (m_modifier.getOnHoverExit()) m_modifier.getOnHoverExit()(this);
        }
        return false;
    }
    if (!m_hovered) {
        m_hovered = true;
        m_dirty = true;
        if (m_modifier.getOnHoverEnter()) m_modifier.getOnHoverEnter()(this);
    }

    if (m_uiloRef) {
        if (m_modifier.hasCursor())
            m_uiloRef->requestCursor(m_modifier.getCursor(), 1);
        else if (m_modifier.getOnLeftClick())
            m_uiloRef->requestCursor(CursorType::Hand, 1);
    }
    return claimsPointerEvents();
}


/*
    checkScroll(const Vec2f& mousePosition, float delta, bool precise, bool momentum):
    - Params:   const Vec2f& mousePosition, float delta, bool precise, bool
                momentum
    - Returns:  bool -- true when the event was consumed
    - Desc:     Fires the Modifier's onScroll handler when the position is
                inside the element. A plain element does not scroll itself, so
                without that handler the event is declined and bubbles to a
                parent that can use it.
*/
bool Element::checkScroll(
    const Vec2f& mousePosition,
    float delta,
    bool /*precise*/,
    bool /*momentum*/
) {
    if (!m_bounds.contains(mousePosition)) return false;
    if (m_modifier.getOnScroll()) {
        m_modifier.getOnScroll()(this, delta);
        return true;
    }
    return false;
}


/*
    setUILO(UILO& uiloRef):
    - Params:   UILO& uiloRef
    - Returns:  void
    - Desc:     Binds the element to a UILO, which puts it in the element pool
                and registers its name for lookup. Re-binding to the same UILO
                is a no-op: switching the active page from A to B and back re-
                walks A's tree, and emplacing the element a second time would
                leave two unique_ptrs owning it and double-delete at shutdown.
*/
void Element::setUILO(UILO& uiloRef) {
    if (m_uiloRef == &uiloRef) return;
    m_uiloRef = &uiloRef;
    uiloRef.m_elementPool.emplace_back(this);
    if (!m_name.empty()) uiloRef.m_elements[m_name] = this;
}

}
