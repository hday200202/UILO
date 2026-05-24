#include "Spacer.hpp"
#include "../../UILO.hpp"
#include "../../utils/RenderUtils.hpp"

namespace uilo {

Spacer::Spacer(
    Modifier modifier,
    SpacerOptions options,
    const std::string& name
) : m_options(options) {
    m_modifier = modifier;
    m_name = name;
}

void Spacer::update(sf::FloatRect& parentBounds, float dt) { resize(parentBounds); (void)dt; }

void Spacer::render(sf::RenderTarget& target) {
    float scale   = m_uiloRef ? m_uiloRef->getScale() : 1.f;
    const float r = m_options.getRounding() * scale;
    const sf::Color c = m_options.getColor();

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