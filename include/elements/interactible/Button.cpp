#include "Button.hpp"
#include "../../UILO.hpp"

namespace uilo {

namespace {

/*
    rowOptionsFrom(const ButtonOptions& o):
    - Params:   const ButtonOptions& o
    - Returns:  RowOptions
    - Desc:     The one place ButtonOptions becomes RowOptions. Three callers
                need the conversion -- the constructor, setOptions, and the per-
                frame sync in render -- and they were once written out
                separately, which is exactly how a newly added property ends up
                wired in one of them and forgotten in the other two. Rounding is
                passed through unresolved so the Theme keeps reaching the row.
*/
RowOptions rowOptionsFrom(const ButtonOptions& o) {
    RowOptions out;
    out.setColor(o.getColor())
       .setColorRole(o.getColorRole())
       .setGradient(o.getGradient())
       .setGradientRole(o.getGradientRole())
       .inheritRounding(o.getRoundingOpt(), o.getRoundingFallback())
       .setOutlineColor(o.getOutlineColor())
       .setOutlineColorRole(o.getOutlineColorRole())
       .setOutlineThickness(o.getOutlineThickness());
    return out;
}

} // namespace


/*
    Button(Modifier modifier, ButtonOptions options, const std::string& name):
    - Params:   Modifier modifier, ButtonOptions options,
                const std::string& name
    - Returns:  Button
    - Desc:     Constructs a button as a Row carrying the converted options, and
                adds the label as its first child when one was given. Tagged as
                a Button so hit-testing and the web bridge can identify it.
*/
Button::Button(
    Modifier modifier,
    ButtonOptions options,
    const std::string& name
) : Row(modifier, rowOptionsFrom(options), {}, name), m_buttonOptions(options) {
    m_type = ElementType::Button;
    if (options.getLabel()) m_children.push_back(options.getLabel());
}


/*
    setOptions(const ButtonOptions& opts):
    - Params:   const ButtonOptions& opts
    - Returns:  void
    - Desc:     Replaces the options, pushes them into the underlying Row, and
                rebuilds the child list around the new label. The child list is
                cleared outright, so anything added beside the label is dropped
                -- a widget that keeps extra children re-adds them afterwards.
*/
void Button::setOptions(const ButtonOptions& opts) {
    m_buttonOptions = opts;
    Row::setOptions(rowOptionsFrom(opts));
    m_children.clear();
    if (opts.getLabel()) m_children.push_back(opts.getLabel());
}


/*
    checkLeftClick(const Vec2f& mousePosition):
    - Params:   const Vec2f& mousePosition
    - Returns:  bool -- true when the button claimed the click
    - Desc:     Uses Element's handling rather than Row's, so the click stops
                here instead of being offered to the label and the other
                children first. A button is one target, not a container of them.
*/
bool Button::checkLeftClick(const Vec2f& mousePosition) {
    return Element::checkLeftClick(mousePosition);
}


/*
    checkRightClick(const Vec2f& mousePosition):
    - Params:   const Vec2f& mousePosition
    - Returns:  bool -- true when the button claimed the click
    - Desc:     As checkLeftClick, for the right button.
*/
bool Button::checkRightClick(const Vec2f& mousePosition) {
    return Element::checkRightClick(mousePosition);
}


/*
    checkHover(const Vec2f& mousePosition):
    - Params:   const Vec2f& mousePosition
    - Returns:  bool -- true when the button claimed the hover
    - Desc:     Asks for the hand cursor whenever the pointer is inside, which
                is unconditional here rather than tied to having a click
                handler: anything drawn as a button should read as clickable.
                The request is made every frame, since UILO clears the pool each
                frame.
*/
bool Button::checkHover(const Vec2f& mousePosition) {
    if (m_bounds.contains(mousePosition) && m_uiloRef)
        m_uiloRef->requestCursor(CursorType::Hand, 1);
    return Element::checkHover(mousePosition);
}


/*
    checkScroll(const Vec2f& mousePosition, float delta, bool precise, bool momentum):
    - Params:   const Vec2f& mousePosition, float delta, bool precise,
                bool momentum
    - Returns:  bool -- true when the event was consumed
    - Desc:     Uses Element's handling, so a button never scrolls itself and a
                wheel over one bubbles to the list or panel it sits in.
*/
bool Button::checkScroll(
    const Vec2f& mousePosition,
    float delta,
    bool precise,
    bool momentum
) {
    return Element::checkScroll(mousePosition, delta, precise, momentum);
}


/*
    render():
    - Params:   none
    - Returns:  void
    - Desc:     Syncs the live ButtonOptions into the underlying Row before
                drawing, so a handler that mutates getOptions() directly -- the
                usual `b->getOptions().setColorRole("accentHover")` from an
                onHoverEnter -- takes effect on the next frame without an
                explicit setOptions() call. The comparison guards against
                rebuilding the RowOptions every frame, and the row's own scroll
                settings are carried across since ButtonOptions does not model
                them.
*/
void Button::render() {
    if (!m_modifier.getVisible()) { m_dirty = false; return; }

    const RowOptions& cur = Row::getOptions();
    if (cur.getColor()            != m_buttonOptions.getColor() ||
        cur.getColorRole()        != m_buttonOptions.getColorRole() ||
        cur.getGradient()         != m_buttonOptions.getGradient() ||
        cur.getGradientRole()     != m_buttonOptions.getGradientRole() ||
        cur.getRoundingOpt()      != m_buttonOptions.getRoundingOpt() ||
        cur.getRoundingFallback() != m_buttonOptions.getRoundingFallback() ||
        cur.getOutlineColor()     != m_buttonOptions.getOutlineColor() ||
        cur.getOutlineColorRole() != m_buttonOptions.getOutlineColorRole() ||
        cur.getOutlineThickness() != m_buttonOptions.getOutlineThickness())
    {
        RowOptions next = rowOptionsFrom(m_buttonOptions);
        next.setScrollable(cur.getScrollable())
            .setScrollSpeed(cur.getScrollSpeed());
        Row::setOptions(next);
    }

    Row::render();
}

}
