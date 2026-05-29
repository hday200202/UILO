#include "Button.hpp"
#include "../../UILO.hpp"

namespace uilo {

Button::Button(Modifier modifier, ButtonOptions options, const std::string& name)
    : Row(modifier, RowOptions().setColor(options.getColor()).setColorRole(options.getColorRole()).setRounding(options.getRounding()), {}, name),
      m_buttonOptions(options)
{
    m_type = ElementType::Button;
    if (options.getLabel()) m_children.push_back(options.getLabel());
}

void Button::setOptions(const ButtonOptions& opts) {
    m_buttonOptions = opts;
    Row::setOptions(RowOptions().setColor(opts.getColor()).setColorRole(opts.getColorRole()).setRounding(opts.getRounding()));
    m_children.clear();
    if (opts.getLabel()) m_children.push_back(opts.getLabel());
}

bool Button::checkLeftClick(const Vec2f& mousePosition) {
    return Element::checkLeftClick(mousePosition);
}

bool Button::checkRightClick(const Vec2f& mousePosition) {
    return Element::checkRightClick(mousePosition);
}

bool Button::checkHover(const Vec2f& mousePosition) {
    if (m_bounds.contains(mousePosition) && m_uiloRef)
        m_uiloRef->requestCursor(CursorType::Hand, 1);
    return Element::checkHover(mousePosition);
}

bool Button::checkScroll(const Vec2f& mousePosition, float delta, bool precise, bool momentum) {
    return Element::checkScroll(mousePosition, delta, precise, momentum);
}

void Button::render() {
    // Push live ButtonOptions into the underlying Row each frame so that
    // mutations made via `getOptions()` from inside a callback (e.g.
    // `b->getOptions().setColor(Color::Red)` from an onHover handler) take
    // effect immediately on the next draw.
    const RowOptions& cur = Row::getOptions();
    if (cur.getColor()    != m_buttonOptions.getColor() ||
        cur.getColorRole() != m_buttonOptions.getColorRole() ||
        cur.getRounding() != m_buttonOptions.getRounding())
    {
        Row::setOptions(RowOptions()
            .setColor(m_buttonOptions.getColor())
            .setColorRole(m_buttonOptions.getColorRole())
            .setRounding(m_buttonOptions.getRounding())
            .setScrollable(cur.getScrollable())
            .setScrollSpeed(cur.getScrollSpeed()));
    }
    Row::render();
}

}
