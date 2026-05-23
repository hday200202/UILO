#include "Button.hpp"

namespace uilo {

Button::Button(Modifier modifier, Text* text, const std::string& name)
    : Row(modifier, {}, name)
{
    m_type = ElementType::Button;
    m_children.push_back(text);
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
