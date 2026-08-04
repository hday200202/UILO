#include "Modifier.hpp"
#include <algorithm>

namespace uilo {

/*
    setWidth(Dimension dim):
    - Params:   Dimension dim
    - Returns:  Modifier&
    - Desc:     Sets the element's width. A percent value is clamped to 1..100,
                since a container distributes percent children against the space
                left over and a value outside that range has no meaning there.
                Pixel values pass through and are scaled at layout time.
*/
Modifier& Modifier::setWidth(Dimension dim) {
    if (dim.percent) dim.value = std::clamp(dim.value, 1.f, 100.f);
    m_width = dim;
    return *this;
}


/*
    setHeight(Dimension dim):
    - Params:   Dimension dim
    - Returns:  Modifier&
    - Desc:     Sets the element's height, clamping a percent value the same way
                setWidth does.
*/
Modifier& Modifier::setHeight(Dimension dim) {
    if (dim.percent) dim.value = std::clamp(dim.value, 1.f, 100.f);
    m_height = dim;
    return *this;
}


/*
    setAlign(Align alignment):
    - Params:   Align alignment
    - Returns:  Modifier&
    - Desc:     Sets where the element sits inside the slot its parent gives it.
                Horizontal and vertical flags combine with `|`.
*/
Modifier& Modifier::setAlign(Align alignment) { m_align = alignment; return *this; }


/*
    setVisible(bool visible):
    - Params:   bool visible
    - Returns:  Modifier&
    - Desc:     Shows or hides the element. A hidden element is skipped by
                layout, takes no space, and is not drawn.
*/
Modifier& Modifier::setVisible(bool visible) { m_visible = visible; return *this; }


/*
    ignoreScroll(bool ignore):
    - Params:   bool ignore
    - Returns:  Modifier&
    - Desc:     Pins the element inside a scrollable parent. A pinned child
                keeps its place and reserves its space, and the parent scrolls
                only the viewport left over -- which is how a header stays put
                above a scrolling list.
*/
Modifier& Modifier::ignoreScroll(bool ignore) { m_ignoreScroll = ignore; return *this; }


/*
    setFreePosition(const Vec2f& freePos):
    - Params:   const Vec2f& freePos
    - Returns:  Modifier&
    - Desc:     Position for an element placed outside the layout flow.
*/
Modifier& Modifier::setFreePosition(const Vec2f& freePos) { m_freePosition = freePos; return *this; }


/*
    setMaterial(const Material& m):
    - Params:   const Material& m
    - Returns:  Modifier&
    - Desc:     Gives the element a material background -- glass and the like. A
                material owns the background: it draws its own rounded rect,
                tint and effect, so the element's flat fill is skipped rather
                than drawn underneath it.
*/
Modifier& Modifier::setMaterial(const Material& m) { m_material = m; return *this; }


/*
    setCursor(CursorType cursor):
    - Params:   CursorType cursor
    - Returns:  Modifier&
    - Desc:     The mouse cursor to show while the pointer is over this element.
                Requested for every frame the element is hovered, not once on
                the way in, so it survives as long as the pointer stays. That is
                the difference from calling UILO::requestCursor() from an
                onHoverEnter handler: the request pool is cleared at the top of
                each frame, so an edge-triggered handler's cursor lasts a single
                frame no matter what priority it asks for.
    - An element with a left-click handler already asks for the hand, so this is
      only needed to pick a different shape, or to force one on an element that
      is not clickable. Setting Arrow explicitly suppresses that automatic hand.
*/
Modifier& Modifier::setCursor(CursorType cursor) { m_cursor = cursor; return *this; }


/*
    clearCursor():
    - Params:   none
    - Returns:  Modifier&
    - Desc:     Drops an explicit cursor, so the element goes back to the
                default behaviour -- the hand when it is clickable, and whatever
                is underneath otherwise.
*/
Modifier& Modifier::clearCursor() { m_cursor.reset(); return *this; }


/*
    getWidth():
    - Params:   none
    - Returns:  Dimension
    - Desc:     The element's declared width, in pixels or percent.
*/
Dimension Modifier::getWidth() const { return m_width; }


/*
    getHeight():
    - Params:   none
    - Returns:  Dimension
    - Desc:     The element's declared height, in pixels or percent.
*/
Dimension Modifier::getHeight() const { return m_height; }


/*
    getAlign():
    - Params:   none
    - Returns:  Align
    - Desc:     The combined alignment flags.
*/
Align Modifier::getAlign() const { return m_align; }


/*
    getOnLeftClick():
    - Params:   none
    - Returns:  const FuncPtr&
    - Desc:     The left-click handler, empty when none was set. Testing it is
                also how an element decides whether it claims pointer events.
*/
const FuncPtr& Modifier::getOnLeftClick() const { return m_onLeftClick; }


/*
    getOnRightClick():
    - Params:   none
    - Returns:  const FuncPtr&
    - Desc:     The right-click handler, empty when none was set.
*/
const FuncPtr& Modifier::getOnRightClick() const { return m_onRightClick; }


/*
    getOnHoverEnter():
    - Params:   none
    - Returns:  const FuncPtr&
    - Desc:     The hover-enter handler, empty when none was set.
*/
const FuncPtr& Modifier::getOnHoverEnter() const { return m_onHoverEnter; }


/*
    getOnHoverExit():
    - Params:   none
    - Returns:  const FuncPtr&
    - Desc:     The hover-exit handler, empty when none was set.
*/
const FuncPtr& Modifier::getOnHoverExit() const { return m_onHoverExit; }


/*
    getOnUpdateStart():
    - Params:   none
    - Returns:  const FuncPtr&
    - Desc:     The start-of-tick hook, empty when none was set.
*/
const FuncPtr& Modifier::getOnUpdateStart() const { return m_onUpdateStart; }


/*
    getOnUpdateEnd():
    - Params:   none
    - Returns:  const FuncPtr&
    - Desc:     The end-of-tick hook, empty when none was set.
*/
const FuncPtr& Modifier::getOnUpdateEnd() const { return m_onUpdateEnd; }


/*
    getOnScroll():
    - Params:   none
    - Returns:  const ScrollFuncPtr&
    - Desc:     The scroll handler, empty when none was set.
*/
const ScrollFuncPtr& Modifier::getOnScroll() const { return m_onScroll; }


/*
    getVisible():
    - Params:   none
    - Returns:  bool
    - Desc:     Whether the element is laid out and drawn.
*/
bool Modifier::getVisible() const { return m_visible; }


/*
    getIgnoreScroll():
    - Params:   none
    - Returns:  bool
    - Desc:     Whether the element is pinned inside a scrollable parent.
*/
bool Modifier::getIgnoreScroll() const { return m_ignoreScroll; }


/*
    getFreePosition():
    - Params:   none
    - Returns:  Vec2f
    - Desc:     The position for an element placed outside the layout flow.
*/
Vec2f Modifier::getFreePosition() const { return m_freePosition; }


/*
    getMaterial():
    - Params:   none
    - Returns:  const Material&
    - Desc:     The element's material. Kind None means it has none, and the
                ordinary background is drawn instead.
*/
const Material& Modifier::getMaterial() const { return m_material; }


/*
    getCursor():
    - Params:   none
    - Returns:  CursorType
    - Desc:     The explicit cursor for this element, Arrow when none was set.
                Ask hasCursor() to tell a deliberate Arrow from the default.
*/
CursorType Modifier::getCursor() const { return m_cursor.value_or(CursorType::Arrow); }


/*
    hasCursor():
    - Params:   none
    - Returns:  bool
    - Desc:     Whether this element names its own cursor, which is what lets an
                explicit Arrow override the hand a clickable element would
                otherwise ask for.
*/
bool Modifier::hasCursor() const { return m_cursor.has_value(); }

} // namespace uilo
