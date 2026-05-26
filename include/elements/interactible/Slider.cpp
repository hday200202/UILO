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

void Slider::update(Rectf& parentBounds, float /*dt*/) {
    resize(parentBounds);

    if (m_dragging) {
        float mx, my;
        uint32_t btns = SDL_GetMouseState(&mx, &my);
        if (!(btns & SDL_BUTTON_MASK(SDL_BUTTON_LEFT))) {
            m_dragging = false;
        } else {
            const bool isHoriz = m_options.getOrientation() == SliderOrientation::Horizontal;
            if (m_uiloRef) m_uiloRef->requestCursor(
                isHoriz ? CursorType::SizeHorizontal : CursorType::SizeVertical, 2);
            const Vec2f mouse = m_uiloRef->getMousePosition();
            applyValue(isHoriz ? valueFromMouseX(mouse.x) : valueFromMouseY(mouse.y));
        }
    }
}

void Slider::render() {
    if (!m_uiloRef) { m_dirty = false; return; }
    auto& renderer = m_uiloRef->getRenderer();
    const float scale = m_uiloRef->getScale();
    const bool isHoriz = m_options.getOrientation() == SliderOrientation::Horizontal;
    const float trackRounding = m_options.getTrackRounding() * scale;

    float t = (m_options.getMax() > m_options.getMin())
        ? (m_value - m_options.getMin()) / (m_options.getMax() - m_options.getMin())
        : 0.f;

    if (isHoriz) {
        const float trackH = m_bounds.size.y * m_options.getTrackThickness();
        const float trackY = m_bounds.position.y + (m_bounds.size.y - trackH) * 0.5f;
        const float trackX = m_bounds.position.x;
        const float trackW = m_bounds.size.x;
        const float hw     = resolveThumbHalfWidth();

        // Background track
        renderer.draw(RoundedRect{{trackX, trackY}, {trackW, trackH},
                                  trackRounding, 8, m_options.getTrackColor()});

        // Fill from left to thumb
        const float fillW = hw + t * (trackW - 2.f * hw);
        if (fillW > 0.f)
            renderer.draw(RoundedRect{{trackX, trackY}, {fillW, trackH},
                                      trackRounding, 8, m_options.getFillColor()});

        // Thumb
        const float thumbCX = trackX + hw + t * (trackW - 2.f * hw);
        const float thumbHH = resolveThumbHalfHeight();
        const float thumbCY = m_bounds.position.y + m_bounds.size.y * 0.5f;
        if (m_options.getThumbShape() == ThumbShape::Circle) {
            renderer.draw(Circle{{thumbCX, thumbCY}, hw, 24, m_options.getThumbColor()});
        } else {
            renderer.draw(RoundedRect{
                {thumbCX - hw, thumbCY - thumbHH}, {hw * 2.f, thumbHH * 2.f},
                m_options.getThumbRounding() * scale, 8, m_options.getThumbColor()});
        }
    } else {
        const float trackW = m_bounds.size.x * m_options.getTrackThickness();
        const float trackX = m_bounds.position.x + (m_bounds.size.x - trackW) * 0.5f;
        const float trackY = m_bounds.position.y;
        const float trackH = m_bounds.size.y;
        const float hh     = resolveThumbHalfHeight();

        // Background track
        renderer.draw(RoundedRect{{trackX, trackY}, {trackW, trackH},
                                  trackRounding, 8, m_options.getTrackColor()});

        // Fill from top to thumb
        const float fillH = hh + t * (trackH - 2.f * hh);
        if (fillH > 0.f)
            renderer.draw(RoundedRect{{trackX, trackY}, {trackW, fillH},
                                      trackRounding, 8, m_options.getFillColor()});

        // Thumb
        const float thumbHW = resolveThumbHalfWidth();
        const float thumbCX = m_bounds.position.x + m_bounds.size.x * 0.5f;
        const float thumbCY = trackY + hh + t * (trackH - 2.f * hh);
        if (m_options.getThumbShape() == ThumbShape::Circle) {
            renderer.draw(Circle{{thumbCX, thumbCY}, hh, 24, m_options.getThumbColor()});
        } else {
            renderer.draw(RoundedRect{
                {thumbCX - thumbHW, thumbCY - hh}, {thumbHW * 2.f, hh * 2.f},
                m_options.getThumbRounding() * scale, 8, m_options.getThumbColor()});
        }
    }

    m_dirty = false;
}

bool Slider::checkHover(const Vec2f& mousePosition) {
    if (m_bounds.contains(mousePosition) && m_uiloRef) {
        const bool isHoriz = m_options.getOrientation() == SliderOrientation::Horizontal;
        m_uiloRef->requestCursor(
            isHoriz ? CursorType::SizeHorizontal : CursorType::SizeVertical, 1);
    }
    return Element::checkHover(mousePosition);
}

bool Slider::checkLeftClick(const Vec2f& mousePosition) {
    if (!m_bounds.contains(mousePosition)) return false;
    m_uiloRef->setCurrInteractible(this);
    m_dragging = true;
    const bool isHoriz = m_options.getOrientation() == SliderOrientation::Horizontal;
    applyValue(isHoriz ? valueFromMouseX(mousePosition.x) : valueFromMouseY(mousePosition.y));
    if (m_modifier.getOnLeftClick()) m_modifier.getOnLeftClick()();
    return true;
}

bool Slider::checkScroll(const Vec2f& mousePosition, float delta) {
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
    m_dirty = true;
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
