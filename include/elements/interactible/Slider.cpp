#include "Slider.hpp"
#include "../../UILO.hpp"
#include "../../utils/RenderUtils.hpp"

#include <algorithm>
#include <cmath>

namespace uilo {

/*
    Slider(Modifier modifier, SliderOptions options, const std::string& name):
    - Params:   Modifier modifier, SliderOptions options,
                const std::string& name
    - Returns:  Slider
    - Desc:     Constructs a slider and seats it at its configured default,
                clamped into range, or at the minimum when no default was given.
*/
Slider::Slider(
    Modifier modifier,
    SliderOptions options,
    const std::string& name
) : m_options(options) {
    m_modifier = modifier;
    m_name     = name;
    m_type     = ElementType::Slider;
    m_value    = m_options.hasDefault()
        ? std::clamp(m_options.getDefaultValue(), m_options.getMin(), m_options.getMax())
        : m_options.getMin();
}

/*
    update(Rectf& parentBounds, float dt):
    - Params:   Rectf& parentBounds, float dt
    - Returns:  void
    - Desc:     Resolves the slider's bounds and, while a drag is running,
                tracks the pointer. The button state is polled rather than
                driven by an event so a release outside the window still ends
                the drag, and the resize cursor is re-requested each frame
                because UILO clears the request pool at the top of every one.
*/
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

/*
    render():
    - Params:   none
    - Returns:  void
    - Desc:     Draws the track, the filled portion up to the thumb, and the
                thumb itself, in whichever orientation the options ask for. A
                vertical slider fills from the bottom up, which is the direction
                a level control is read in. The track's thickness is a fraction
                of the cross axis when it is 1 or below and a pixel count
                otherwise, so the same options work at any element size.
*/
void Slider::render() {
    if (!m_modifier.getVisible()) { m_dirty = false; return; }
    if (!m_uiloRef) { m_dirty = false; return; }
    auto& renderer = m_uiloRef->getRenderer();
    const float scale = m_uiloRef->getScale();
    const bool isHoriz = m_options.getOrientation() == SliderOrientation::Horizontal;
    const float trackRounding = m_options.getTrackRounding() * scale;
    const Color trackColor = resolveColor(m_options.getTrackColorRole(), m_options.getTrackColor());
    const Color fillColor  = resolveColor(m_options.getFillColorRole(),  m_options.getFillColor());
    const Color thumbColor = resolveColor(m_options.getThumbColorRole(), m_options.getThumbColor());

    float t = (m_options.getMax() > m_options.getMin())
        ? (m_value - m_options.getMin()) / (m_options.getMax() - m_options.getMin())
        : 0.f;

    if (isHoriz) {
        const float thickness = m_options.getTrackThickness();
        const float trackH = thickness > 1.f
            ? std::clamp(thickness * scale, 1.f, m_bounds.size.y)
            : std::clamp(m_bounds.size.y * thickness, 1.f, m_bounds.size.y);
        const float trackY = m_bounds.position.y + (m_bounds.size.y - trackH) * 0.5f;
        const float trackX = m_bounds.position.x;
        const float trackW = m_bounds.size.x;
        const float hw     = resolveThumbHalfWidth();

        /* Background track. */
        renderer.draw(RoundedRect{{trackX, trackY}, {trackW, trackH},
                                  trackRounding, 8, trackColor});

        /* Fill, from the left edge to the thumb. */
        const float fillW = hw + t * (trackW - 2.f * hw);
        if (fillW > 0.f)
            renderer.draw(RoundedRect{{trackX, trackY}, {fillW, trackH},
                                      trackRounding, 8, fillColor});

        /* Thumb. */
        const float thumbCX = trackX + hw + t * (trackW - 2.f * hw);
        const float thumbHH = resolveThumbHalfHeight();
        const float thumbCY = m_bounds.position.y + m_bounds.size.y * 0.5f;
        if (m_options.getThumbShape() == ThumbShape::Circle) {
            renderer.draw(Circle{{thumbCX, thumbCY}, hw, 24, thumbColor});
        } else {
            renderer.draw(RoundedRect{
                {thumbCX - hw, thumbCY - thumbHH}, {hw * 2.f, thumbHH * 2.f},
                m_options.getThumbRounding() * scale, 8, thumbColor});
        }
    } else {
        const float thickness = m_options.getTrackThickness();
        const float trackW = thickness > 1.f
            ? std::clamp(thickness * scale, 1.f, m_bounds.size.x)
            : std::clamp(m_bounds.size.x * thickness, 1.f, m_bounds.size.x);
        const float trackX = m_bounds.position.x + (m_bounds.size.x - trackW) * 0.5f;
        const float trackY = m_bounds.position.y;
        const float trackH = m_bounds.size.y;
        const float hh     = resolveThumbHalfHeight();

        /* Background track. */
        renderer.draw(RoundedRect{{trackX, trackY}, {trackW, trackH},
                                  trackRounding, 8, trackColor});

        /* Fill, from the bottom up. */
        const float fillH = hh + t * (trackH - 2.f * hh);
        if (fillH > 0.f)
            renderer.draw(RoundedRect{{trackX, trackY + (trackH - fillH)}, {trackW, fillH},
                                      trackRounding, 8, fillColor});

        /* Thumb. */
        const float thumbHW = resolveThumbHalfWidth();
        const float thumbCX = m_bounds.position.x + m_bounds.size.x * 0.5f;
        const float thumbCY = trackY + trackH - hh - t * (trackH - 2.f * hh);
        if (m_options.getThumbShape() == ThumbShape::Circle) {
            renderer.draw(Circle{{thumbCX, thumbCY}, hh, 24, thumbColor});
        } else {
            renderer.draw(RoundedRect{
                {thumbCX - thumbHW, thumbCY - hh}, {thumbHW * 2.f, hh * 2.f},
                m_options.getThumbRounding() * scale, 8, thumbColor});
        }
    }

    m_dirty = false;
}

/*
    checkHover(const Vec2f& mousePosition):
    - Params:   const Vec2f& mousePosition
    - Returns:  bool -- true when the pointer is over the slider
    - Desc:     Asks for the resize cursor along the slider's own axis while the
                pointer is inside, so the control reads as draggable.
*/
bool Slider::checkHover(const Vec2f& mousePosition) {
    if (m_bounds.contains(mousePosition) && m_uiloRef) {
        const bool isHoriz = m_options.getOrientation() == SliderOrientation::Horizontal;
        m_uiloRef->requestCursor(
            isHoriz ? CursorType::SizeHorizontal : CursorType::SizeVertical, 1);
    }
    return Element::checkHover(mousePosition);
}

/*
    checkLeftClick(const Vec2f& mousePosition):
    - Params:   const Vec2f& mousePosition
    - Returns:  bool -- true when the slider took the click
    - Desc:     Jumps the value to wherever the track was clicked and starts a
                drag, so pressing and dragging are one gesture rather than
                requiring the thumb to be grabbed exactly. A double click within
                the usual window restores the configured default instead.
*/
bool Slider::checkLeftClick(const Vec2f& mousePosition) {
    if (!m_bounds.contains(mousePosition)) return false;

    /* Double click within the usual window restores the default. */
    const uint64_t now = SDL_GetTicks();
    const bool isDouble = (now - m_lastClickMs) < 350;
    m_lastClickMs = now;

    m_uiloRef->setCurrInteractible(this);
    if (isDouble && m_options.hasDefault()) {
        applyValue(m_options.getDefaultValue());
        m_dragging = false;
        if (m_modifier.getOnLeftClick()) m_modifier.getOnLeftClick()(this);
        return true;
    }

    m_dragging = true;
    const bool isHoriz = m_options.getOrientation() == SliderOrientation::Horizontal;
    applyValue(isHoriz ? valueFromMouseX(mousePosition.x) : valueFromMouseY(mousePosition.y));
    if (m_modifier.getOnLeftClick()) m_modifier.getOnLeftClick()(this);
    return true;
}

/*
    checkScroll(const Vec2f& mousePosition, float delta, bool precise, bool momentum):
    - Params:   const Vec2f& mousePosition, float delta, bool precise,
                bool momentum
    - Returns:  bool -- true when the slider consumed the event
    - Desc:     Adjusts the value by the wheel, accumulating the delta so a
                stepped slider still responds to a trackpad whose individual
                deltas are too small to cross an increment. Overscroll at either
                end is discarded rather than banked, so reversing direction
                moves the value immediately instead of first unwinding hidden
                accumulation.
*/
bool Slider::checkScroll(
    const Vec2f& mousePosition,
    float delta,
    bool /*precise*/,
    bool /*momentum*/
) {
    if (!m_bounds.contains(mousePosition)) return false;
    const float range = m_options.getMax() - m_options.getMin();
    const float step  = m_options.getStep() > 0.f
        ? m_options.getStep()
        : range * 0.05f;
    const float scrollSign = m_options.getInvertScroll() ? 1.f : -1.f;
    const float rawDelta = scrollSign * delta * step;
    const float minV = m_options.getMin();
    const float maxV = m_options.getMax();

    const float before = m_value;
    const float target = std::clamp(before + m_scrollAccum + rawDelta, minV, maxV);
    applyValue(target);
    const float applied = m_value - before;
    m_scrollAccum += rawDelta - applied;

    /* At a boundary, drop the overscroll rather than banking it, so reversing
       moves the value at once. */
    if ((m_value <= minV && m_scrollAccum < 0.f) ||
        (m_value >= maxV && m_scrollAccum > 0.f)) {
        m_scrollAccum = 0.f;
    }

    if (std::abs(m_scrollAccum) < 1e-6f) {
        m_scrollAccum = 0.f;
    }
    if (m_modifier.getOnScroll()) m_modifier.getOnScroll()(this, delta);
    return true;
}

/*
    onDeactivate():
    - Params:   none
    - Returns:  void
    - Desc:     Ends any drag when focus moves elsewhere, so the thumb cannot
                stay latched to the pointer after a click lands on something
                else.
*/
void Slider::onDeactivate() { m_dragging = false; }


/*
    setValue(float value):
    - Params:   float value
    - Returns:  void
    - Desc:     Sets the value programmatically, through the same clamping,
                snapping and change-callback path a drag uses.
*/
void Slider::setValue(float value) { applyValue(value); }

/*
    valueFromMouseX(float mouseX):
    - Params:   float mouseX
    - Returns:  float -- the value that x position represents
    - Desc:     Maps a window x onto the slider's range. The travel is measured
                between the thumb's two extreme centres rather than the full
                width, so dragging to either end puts the thumb's edge flush
                with the track's rather than hanging past it.
*/
float Slider::valueFromMouseX(float mouseX) const {
    const float hw    = resolveThumbHalfWidth();
    const float left  = m_bounds.position.x + hw;
    const float width = m_bounds.size.x - 2.f * hw;
    if (width <= 0.f) return m_options.getMin();
    float t = (mouseX - left) / width;
    t = std::clamp(t, 0.f, 1.f);
    return m_options.getMin() + t * (m_options.getMax() - m_options.getMin());
}

/*
    applyValue(float raw):
    - Params:   float raw
    - Returns:  void
    - Desc:     The single place the value changes: clamps into range, snaps to
                the step when one is set, and fires onValueChanged only when the
                result actually differs. That guard is what lets a drag call
                this every frame without flooding a handler.
*/
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

/*
    resolveThumbHalfWidth():
    - Params:   none
    - Returns:  float
    - Desc:     Half the thumb's drawn width in render pixels. A circular thumb
                with no size set falls back to a fraction of the element's cross
                axis, so a slider given only a height still gets a proportionate
                thumb.
*/
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

/*
    resolveThumbHalfHeight():
    - Params:   none
    - Returns:  float
    - Desc:     Half the thumb's drawn height, resolved the same way as the
                width, against the other axis.
*/
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

/*
    valueFromMouseY(float mouseY):
    - Params:   float mouseY
    - Returns:  float -- the value that y position represents
    - Desc:     Maps a window y onto the range for a vertical slider, inverted
                so the top of the track is the maximum.
*/
float Slider::valueFromMouseY(float mouseY) const {
    const float hh     = resolveThumbHalfHeight();
    const float top    = m_bounds.position.y + hh;
    const float height = m_bounds.size.y - 2.f * hh;
    if (height <= 0.f) return m_options.getMin();
    float t = 1.f - ((mouseY - top) / height);
    t = std::clamp(t, 0.f, 1.f);
    return m_options.getMin() + t * (m_options.getMax() - m_options.getMin());
}

}
