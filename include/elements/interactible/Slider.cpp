#include "Slider.hpp"
#include "../../UILO.hpp"
#include "../../utils/RenderUtils.hpp"

#include <algorithm>
#include <cmath>

namespace uilo {

Slider::Slider(Modifier modifier, SliderOptions options, const std::string& name)
    : m_options(options)
{
    m_modifier = modifier;
    m_name     = name;
    m_type     = ElementType::Slider;
    m_value    = m_options.getMin();
}

void Slider::update(sf::FloatRect& parentBounds, float /*dt*/) {
    resize(parentBounds);

    if (m_dragging) {
        if (!sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
            m_dragging = false;
        } else {
            applyValue(valueFromMouseX(m_uiloRef->getMousePosition().x));
        }
    }
}

void Slider::render(sf::RenderTarget& target) {
    if (!m_modifier.getVisible()) return;

    const sf::Vector2f pos  = m_bounds.position;
    const sf::Vector2f size = m_bounds.size;

    const float hw        = resolveThumbHalfWidth();
    const float trackH    = size.y * m_options.getTrackThickness();
    const float trackY    = pos.y + (size.y - trackH) * 0.5f;
    const float trackLeft = pos.x + hw;
    const float trackW    = size.x - 2.f * hw;

    float t = (m_options.getMax() > m_options.getMin())
        ? (m_value - m_options.getMin()) / (m_options.getMax() - m_options.getMin())
        : 0.f;
    t = std::clamp(t, 0.f, 1.f);

    const float thumbX = trackLeft + t * trackW;
    const float r      = m_options.getTrackRounding();

    if (r <= 0.f) {
        sf::RectangleShape bg({trackW, trackH});
        bg.setPosition({trackLeft, trackY});
        bg.setFillColor(m_options.getTrackColor());
        target.draw(bg);
    } else {
        sf::ConvexShape bg = makeRoundedRect({trackLeft, trackY}, {trackW, trackH}, r);
        bg.setFillColor(m_options.getTrackColor());
        target.draw(bg);
    }

    const float fillW = thumbX - trackLeft;
    if (fillW > 0.f) {
        if (r <= 0.f) {
            sf::RectangleShape fill({fillW, trackH});
            fill.setPosition({trackLeft, trackY});
            fill.setFillColor(m_options.getFillColor());
            target.draw(fill);
        } else {
            sf::ConvexShape fill = makeRoundedRect({trackLeft, trackY}, {fillW, trackH}, r);
            fill.setFillColor(m_options.getFillColor());
            target.draw(fill);
        }
    }

    if (m_options.getThumbShape() == ThumbShape::Circle) {
        const float radius = m_options.getThumbSize().x > 0.f
            ? m_options.getThumbSize().x * 0.5f
            : size.y * 0.4f;
        sf::CircleShape thumb(radius);
        thumb.setOrigin({radius, radius});
        thumb.setPosition({thumbX, pos.y + size.y * 0.5f});
        thumb.setFillColor(m_options.getThumbColor());
        target.draw(thumb);
    } else {
        const float tw = m_options.getThumbSize().x;
        const float th = m_options.getThumbSize().y > 0.f ? m_options.getThumbSize().y : size.y;
        const float tr = m_options.getThumbRounding();
        if (tr <= 0.f) {
            sf::RectangleShape thumb({tw, th});
            thumb.setOrigin({tw * 0.5f, th * 0.5f});
            thumb.setPosition({thumbX, pos.y + size.y * 0.5f});
            thumb.setFillColor(m_options.getThumbColor());
            target.draw(thumb);
        } else {
            sf::ConvexShape thumb = makeRoundedRect(
                {thumbX - tw * 0.5f, pos.y + size.y * 0.5f - th * 0.5f},
                {tw, th}, tr
            );
            thumb.setFillColor(m_options.getThumbColor());
            target.draw(thumb);
        }
    }
}

bool Slider::checkLeftClick(const sf::Vector2f& mousePosition) {
    if (!m_bounds.contains(mousePosition)) return false;
    m_uiloRef->setCurrInteractible(this);
    m_dragging = true;
    applyValue(valueFromMouseX(mousePosition.x));
    if (m_modifier.getOnLeftClick()) m_modifier.getOnLeftClick()();
    return true;
}

bool Slider::checkScroll(const sf::Vector2f& mousePosition, float delta) {
    if (!m_bounds.contains(mousePosition)) return false;
    const float range = m_options.getMax() - m_options.getMin();
    const float step  = m_options.getStep() > 0.f
        ? m_options.getStep()
        : range * 0.05f;
    applyValue(m_value + delta * step);
    if (m_modifier.getOnScroll()) m_modifier.getOnScroll()(delta);
    return true;
}

void Slider::onDeactivate() { m_dragging = false; }
void Slider::setValue(float value) { applyValue(value); }

float Slider::valueFromMouseX(float mouseX) const {
    const float hw    = resolveThumbHalfWidth();
    const float left  = m_bounds.position.x + hw;
    const float width = m_bounds.size.x - 2.f * hw;
    if (width <= 0.f) return m_options.getMin();
    float t = (mouseX - left) / width;
    t = std::clamp(t, 0.f, 1.f);
    return m_options.getMin() + t * (m_options.getMax() - m_options.getMin());
}

void Slider::applyValue(float raw) {
    float v = std::clamp(raw, m_options.getMin(), m_options.getMax());
    if (m_options.getStep() > 0.f) {
        v = m_options.getMin()
            + std::round((v - m_options.getMin()) / m_options.getStep())
            * m_options.getStep();
        v = std::clamp(v, m_options.getMin(), m_options.getMax());
    }
    if (v == m_value) return;
    m_value = v;
    if (m_options.getOnValueChanged()) m_options.getOnValueChanged()(m_value);
}

float Slider::resolveThumbHalfWidth() const {
    if (m_options.getThumbShape() == ThumbShape::Circle) {
        const float r = m_options.getThumbSize().x > 0.f
            ? m_options.getThumbSize().x * 0.5f
            : m_bounds.size.y * 0.4f;
        return r;
    }
    return m_options.getThumbSize().x * 0.5f;
}

}
