#include "Button.hpp"
#include "../../UILO.hpp"

namespace uilo {

Button::Button(Modifier modifier, ButtonOptions options, const std::string& name)
    : Row(modifier, RowOptions().setColor(options.getColor()).setRounding(options.getRounding()), {}, name),
      m_buttonOptions(options)
{
    m_type = ElementType::Button;
    if (options.getLabel()) m_children.push_back(options.getLabel());
}

void Button::setOptions(const ButtonOptions& opts) {
    m_buttonOptions = opts;
    Row::setOptions(RowOptions().setColor(opts.getColor()).setRounding(opts.getRounding()));
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

bool Button::checkScroll(const Vec2f& mousePosition, float delta) {
    return Element::checkScroll(mousePosition, delta);
}

}
