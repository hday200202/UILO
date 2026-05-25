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
            const bool isHoriz = m_options.getOrientation() == SliderOrientation::Horizontal;
            if (m_uiloRef) m_uiloRef->requestCursor(
                isHoriz ? sf::Cursor::Type::SizeHorizontal : sf::Cursor::Type::SizeVertical, 2);
            const sf::Vector2f mouse = m_uiloRef->getMousePosition();
            applyValue(isHoriz ? valueFromMouseX(mouse.x) : valueFromMouseY(mouse.y));
        }
    }
}

void Slider::render(sf::RenderTarget& target) {
    if (!m_modifier.getVisible()) return;

    float scale = m_uiloRef ? m_uiloRef->getScale() : 1.f;
    const sf::Vector2f pos  = m_bounds.position;
    const sf::Vector2f size = m_bounds.size;

    float t = (m_options.getMax() > m_options.getMin())
        ? (m_value - m_options.getMin()) / (m_options.getMax() - m_options.getMin())
        : 0.f;
    t = std::clamp(t, 0.f, 1.f);

    if (m_options.getOrientation() == SliderOrientation::Vertical) {
        const float hh        = resolveThumbHalfHeight();
        const float trackW    = size.x * m_options.getTrackThickness();
        const float trackX    = pos.x + (size.x - trackW) * 0.5f;
        const float trackTop  = pos.y + hh;
        const float trackH    = size.y - 2.f * hh;
        const float thumbY    = trackTop + t * trackH;
        const float r         = m_options.getTrackRounding() * scale;

        // Background track
        if (r <= 0.f) {
            sf::RectangleShape bg({trackW, trackH});
            bg.setPosition({trackX, trackTop});
            bg.setFillColor(m_options.getTrackColor());
            target.draw(bg);
        } else {
            sf::ConvexShape bg = makeRoundedRect({trackX, trackTop}, {trackW, trackH}, r);
            bg.setFillColor(m_options.getTrackColor());
            target.draw(bg);
        }

        // Fill (top to thumb)
        const float fillH = thumbY - trackTop;
        if (fillH > 0.f) {
            if (r <= 0.f) {
                sf::RectangleShape fill({trackW, fillH});
                fill.setPosition({trackX, trackTop});
                fill.setFillColor(m_options.getFillColor());
                target.draw(fill);
            } else {
                sf::ConvexShape fill = makeRoundedRect({trackX, trackTop}, {trackW, fillH}, r);
                fill.setFillColor(m_options.getFillColor());
                target.draw(fill);
            }
        }

        // Thumb
        const float cx = pos.x + size.x * 0.5f;
        if (m_options.getThumbShape() == ThumbShape::Circle) {
            const float radius = m_options.getThumbSize().x > 0.f
                ? m_options.getThumbSize().x * scale * 0.5f
                : size.x * 0.4f;
            sf::CircleShape thumb(radius);
            thumb.setOrigin({radius, radius});
            thumb.setPosition({cx, thumbY});
            thumb.setFillColor(m_options.getThumbColor());
            target.draw(thumb);
        } else {
            const float tw = m_options.getThumbSize().x > 0.f ? m_options.getThumbSize().x * scale : size.x;
            const float th = m_options.getThumbSize().y > 0.f ? m_options.getThumbSize().y * scale : size.x;
            const float tr = m_options.getThumbRounding() * scale;
            if (tr <= 0.f) {
                sf::RectangleShape thumb({tw, th});
                thumb.setOrigin({tw * 0.5f, th * 0.5f});
                thumb.setPosition({cx, thumbY});
                thumb.setFillColor(m_options.getThumbColor());
                target.draw(thumb);
            } else {
                sf::ConvexShape thumb = makeRoundedRect(
                    {cx - tw * 0.5f, thumbY - th * 0.5f}, {tw, th}, tr);
                thumb.setFillColor(m_options.getThumbColor());
                target.draw(thumb);
            }
        }
        return;
    }

    // Horizontal path
    const float hw        = resolveThumbHalfWidth();
    const float trackH    = size.y * m_options.getTrackThickness();
    const float trackY    = pos.y + (size.y - trackH) * 0.5f;
    const float trackLeft = pos.x + hw;
    const float trackW    = size.x - 2.f * hw;

    const float thumbX = trackLeft + t * trackW;
    const float r      = m_options.getTrackRounding() * scale;

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
            ? m_options.getThumbSize().x * scale * 0.5f
            : size.y * 0.4f;
        sf::CircleShape thumb(radius);
        thumb.setOrigin({radius, radius});
        thumb.setPosition({thumbX, pos.y + size.y * 0.5f});
        thumb.setFillColor(m_options.getThumbColor());
        target.draw(thumb);
    } else {
        const float tw = m_options.getThumbSize().x * scale;
        const float th = m_options.getThumbSize().y > 0.f ? m_options.getThumbSize().y * scale : size.y;
        const float tr = m_options.getThumbRounding() * scale;
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

bool Slider::checkHover(const sf::Vector2f& mousePosition) {
    if (m_bounds.contains(mousePosition) && m_uiloRef) {
        const bool isHoriz = m_options.getOrientation() == SliderOrientation::Horizontal;
        m_uiloRef->requestCursor(
            isHoriz ? sf::Cursor::Type::SizeHorizontal : sf::Cursor::Type::SizeVertical, 1);
    }
    return Element::checkHover(mousePosition);
}

bool Slider::checkLeftClick(const sf::Vector2f& mousePosition) {
    if (!m_bounds.contains(mousePosition)) return false;
    m_uiloRef->setCurrInteractible(this);
    m_dragging = true;
    const bool isHoriz = m_options.getOrientation() == SliderOrientation::Horizontal;
    applyValue(isHoriz ? valueFromMouseX(mousePosition.x) : valueFromMouseY(mousePosition.y));
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
    float scale = m_uiloRef ? m_uiloRef->getScale() : 1.f;
    if (m_options.getThumbShape() == ThumbShape::Circle) {
        const float r = m_options.getThumbSize().x > 0.f
            ? m_options.getThumbSize().x * scale * 0.5f
            : m_bounds.size.y * 0.4f;
        return r;
    }
    return m_options.getThumbSize().x * scale * 0.5f;
}

float Slider::resolveThumbHalfHeight() const {
    float scale = m_uiloRef ? m_uiloRef->getScale() : 1.f;
    if (m_options.getThumbShape() == ThumbShape::Circle) {
        const float r = m_options.getThumbSize().x > 0.f
            ? m_options.getThumbSize().x * scale * 0.5f
            : m_bounds.size.x * 0.4f;
        return r;
    }
    return m_options.getThumbSize().y * scale * 0.5f;
}

float Slider::valueFromMouseY(float mouseY) const {
    const float hh     = resolveThumbHalfHeight();
    const float top    = m_bounds.position.y + hh;
    const float height = m_bounds.size.y - 2.f * hh;
    if (height <= 0.f) return m_options.getMin();
    float t = (mouseY - top) / height;
    t = std::clamp(t, 0.f, 1.f);
    return m_options.getMin() + t * (m_options.getMax() - m_options.getMin());
}

}
