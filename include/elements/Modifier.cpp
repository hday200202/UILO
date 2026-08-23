#include "Modifier.hpp"
#include <algorithm>

namespace uilo {

/*
    setWidth(Dimension dim):
    - Params:   Dimension dim
    - Returns:  Modifier&
    - Desc:     Sets the element's width. A percent value is clamped to 1..100,
                so a percentage can never ask for more of the parent than the
                parent has; pixel values pass through and are scaled at layout
                time, and a _flex share passes through untouched since its value
                is a weight against its siblings rather than a size.
*/
Modifier& Modifier::setWidth(Dimension dim) {
    if (dim.percent) dim.value = std::clamp(dim.value, 1.f, 100.f);
    m_width.set(dim);
    return *this;
}


/*
    setHeight(Dimension dim):
    - Params:   Dimension dim
    - Returns:  Modifier&
    - Desc:     Sets the element's height, clamping a percent value and passing an
                _flex share through the same way setWidth does.
*/
Modifier& Modifier::setHeight(Dimension dim) {
    if (dim.percent) dim.value = std::clamp(dim.value, 1.f, 100.f);
    m_height.set(dim);
    return *this;
}


/*
    setAlign(Align alignment):
    - Params:   Align alignment
    - Returns:  Modifier&
    - Desc:     Sets where the element sits inside the slot its parent gives it.
                Horizontal and vertical flags combine with `|`.
*/
Modifier& Modifier::setAlign(Align alignment) { m_align.set(alignment); return *this; }


/*
    setVisible(bool visible):
    - Params:   bool visible
    - Returns:  Modifier&
    - Desc:     Shows or hides the element. A hidden element is skipped by
                layout, takes no space, and is not drawn.
*/
Modifier& Modifier::setVisible(bool visible) { m_visible.set(visible); return *this; }


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
Modifier& Modifier::setFreePosition(const Vec2f& freePos) {
    m_freeX = Dimension{freePos.x, false};
    m_freeY = Dimension{freePos.y, false};
    return *this;
}


/*
    setFreePosition(Dimension x, Dimension y):
    - Params:   Dimension x, Dimension y
    - Returns:  Modifier&
    - Desc:     Where a floating element sits, measured from its container's
                content corner. A percent dimension is taken against the
                container's own size, so a panel can sit at 50_pct and stay
                centred as the pane resizes. Ignored unless setFloating.
*/
Modifier& Modifier::setFreePosition(Dimension x, Dimension y) {
    m_freeX = x;
    m_freeY = y;
    return *this;
}


/*
    setFloating(bool floating):
    - Params:   bool floating
    - Returns:  Modifier&
    - Desc:     Takes the element out of its container's flow. It consumes no
                space, ignores setAlign, and is placed at its free position
                instead; everything else about being a child is unchanged, so it
                is updated in tree order and clipped by its container.
*/
Modifier& Modifier::setFloating(bool floating) { m_floating = floating; return *this; }


/*
    setDraggable(bool draggable):
    - Params:   bool draggable
    - Returns:  Modifier&
    - Desc:     Lets the pointer move a floating element by dragging it, which
                writes back to its free position. Ignored unless the element is
                floating, since a child in the flow is positioned by its
                container.
*/
Modifier& Modifier::setDraggable(bool draggable) { m_draggable = draggable; return *this; }


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
    setContextMenu(std::vector<ContextMenuItem> items):
    - Params:   std::vector<ContextMenuItem> items
    - Returns:  Modifier&
    - Desc:     Declares a fixed context menu. Stored as a builder that hands
                back a copy, so the two spellings share one path through the
                dispatch and neither needs a special case.
*/
Modifier& Modifier::setContextMenu(std::vector<ContextMenuItem> items) {
    if (items.empty()) return clearContextMenu();
    m_contextMenu = [items = std::move(items)] { return items; };
    return *this;
}


/*
    setContextMenu(ContextMenuBuilder builder):
    - Params:   ContextMenuBuilder builder
    - Returns:  Modifier&
    - Desc:     Declares a context menu built on demand. The builder runs on
                every right-click, on the frame the click is dispatched, so it
                sees current application state and may return nothing to decline
                the menu entirely.
*/
Modifier& Modifier::setContextMenu(ContextMenuBuilder builder) {
    m_contextMenu = std::move(builder);
    return *this;
}


/*
    clearContextMenu():
    - Params:   none
    - Returns:  Modifier&
    - Desc:     Drops the menu, so a right-click passes through to whatever is
                behind this element instead of being consumed.
*/
Modifier& Modifier::clearContextMenu() {
    m_contextMenu = nullptr;
    return *this;
}


/*
    hasContextMenu():
    - Params:   none
    - Returns:  bool
    - Desc:     Whether a menu was declared. Says nothing about whether the
                builder will actually produce items -- only buildContextMenu()
                can answer that, and it costs whatever the builder costs.
*/
bool Modifier::hasContextMenu() const { return static_cast<bool>(m_contextMenu); }


/*
    buildContextMenu():
    - Params:   none
    - Returns:  std::vector<ContextMenuItem>
    - Desc:     The items to show, empty when there is no menu or the builder
                declined.
*/
std::vector<ContextMenuItem> Modifier::buildContextMenu() const {
    if (!m_contextMenu) return {};
    return m_contextMenu();
}


/*
    getWidth():
    - Params:   none
    - Returns:  Dimension
    - Desc:     The element's declared width, in pixels or percent.
*/
Dimension Modifier::getWidth() const { return m_width.get(); }


/*
    getHeight():
    - Params:   none
    - Returns:  Dimension
    - Desc:     The element's declared height, in pixels or percent.
*/
Dimension Modifier::getHeight() const { return m_height.get(); }


/*
    getAlign():
    - Params:   none
    - Returns:  Align
    - Desc:     The combined alignment flags.
*/
Align Modifier::getAlign() const { return m_align.get(); }


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
bool Modifier::getVisible() const { return m_visible.get(); }


/*
    getIgnoreScroll():
    - Params:   none
    - Returns:  bool
    - Desc:     Whether the element is pinned inside a scrollable parent.
*/
bool Modifier::getIgnoreScroll() const { return m_ignoreScroll; }


/*
    getFreeX() / getFreeY():
    - Params:   none
    - Returns:  Vec2f
    - Desc:     The position for an element placed outside the layout flow.
*/
Dimension Modifier::getFreeX() const { return m_freeX; }
Dimension Modifier::getFreeY() const { return m_freeY; }


/*
    isFloating():
    - Params:   none
    - Returns:  bool
    - Desc:     Whether the element is outside its container's flow.
*/
bool Modifier::isFloating() const { return m_floating; }


/*
    isDraggable():
    - Params:   none
    - Returns:  bool
    - Desc:     Whether a floating element may be moved by the pointer.
*/
bool Modifier::isDraggable() const { return m_draggable; }


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
