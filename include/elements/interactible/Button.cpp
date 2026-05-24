#include "Button.hpp"

namespace uilo {

Button::Button(Modifier modifier, ButtonOptions options, const std::string& name)
    : Row(modifier, {}, name)
{
    m_type = ElementType::Button;
    if (options.getLabel()) m_children.push_back(options.getLabel());
}

bool Button::checkLeftClick(const sf::Vector2f& mousePosition) {
    return Element::checkLeftClick(mousePosition);
}

bool Button::checkRightClick(const sf::Vector2f& mousePosition) {
    return Element::checkRightClick(mousePosition);
}

bool Button::checkHover(const sf::Vector2f& mousePosition) {
    return Element::checkHover(mousePosition);
}

bool Button::checkScroll(const sf::Vector2f& mousePosition, float delta) {
    return Element::checkScroll(mousePosition, delta);
}

}
