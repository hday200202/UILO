#include "Spacer.hpp"
#include "../../utils/RenderUtils.hpp"

namespace uilo {

Spacer::Spacer(
    Modifier modifier,
    SpacerOptions /*options*/,
    const std::string& name
) {
    m_modifier = modifier;
    m_name = name;
}

void Spacer::update(sf::FloatRect& parentBounds, float dt) { resize(parentBounds); }

void Spacer::render(sf::RenderTarget& target) {
    const float r = m_modifier.getRounding();
    const sf::Color c = m_modifier.getColor();

    if (r <= 0.f) {
        sf::RectangleShape rect;
        rect.setSize(m_bounds.size);
        rect.setPosition(m_bounds.position);
        rect.setFillColor(c);
        target.draw(rect);
    } else {
        sf::ConvexShape rounded = makeRoundedRect(m_bounds.position, m_bounds.size, r);
        rounded.setFillColor(c);
        target.draw(rounded);
    }
}

}