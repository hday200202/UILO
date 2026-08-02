#include "Knob.hpp"
#include "../../UILO.hpp"
#include "../../utils/RenderUtils.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace uilo {

namespace {
constexpr float kPi      = 3.14159265358979323846f;
constexpr float kDeg2Rad = kPi / 180.f;

/*
    wrap360(float a):
    - Params:   float a
    - Returns:  float -- the angle in [0, 360)
    - Desc:     Normalises an angle in degrees, keeping the result positive so a
                negative input still lands in range.
*/
inline float wrap360(float a) {
    a = std::fmod(a, 360.f);
    if (a < 0.f) a += 360.f;
    return a;
}
} // namespace

/*
    Knob(Modifier modifier, KnobOptions options, const std::string& name):
    - Params:   Modifier modifier, KnobOptions options, const std::string& name
    - Returns:  Knob
    - Desc:     Constructs a knob and seats it at its configured default,
                clamped into range, or at the minimum when no default was given.
*/
Knob::Knob(
    Modifier modifier,
    KnobOptions options,
    const std::string& name
) : m_options(options) {
    m_modifier = modifier;
    m_name     = name;
    m_type     = ElementType::Knob;
    m_value    = m_options.hasDefault()
        ? std::clamp(m_options.getDefaultValue(), m_options.getMin(), m_options.getMax())
        : m_options.getMin();
}

/*
    sweepDegrees():
    - Params:   none
    - Returns:  float -- signed total sweep, positive counter-clockwise
    - Desc:     How far the arc travels from the start angle to the end angle
                along the configured direction. The sign carries the direction,
                so callers can interpolate without branching on it. A start and
                end that coincide would otherwise give a zero-length arc and
                draw nothing, so that case is taken as a full revolution instead
                -- which is what someone asking for 0 to 0 means.
*/
float Knob::sweepDegrees() const {
    float diff = m_options.getEndAngle() - m_options.getStartAngle();
    if (m_options.getArcDirection() == KnobArcDir::CounterClockwise) {
        diff = wrap360(diff);
    } else {
        diff = -wrap360(-diff);
    }
    /* A degenerate sweep becomes a full revolution, so start == end draws a
       ring rather than nothing. */
    if (std::abs(diff) < 0.001f) {
        diff = (m_options.getArcDirection() == KnobArcDir::CounterClockwise) ? 360.f : -360.f;
    }
    return diff;
}

/*
    angleForValue(float v):
    - Params:   float v
    - Returns:  float -- cartesian angle in degrees
    - Desc:     Where a value sits along the arc. A degenerate range pins to the
                start angle rather than dividing by zero.
*/
float Knob::angleForValue(float v) const {
    const float range = m_options.getMax() - m_options.getMin();
    const float t = (range > 0.f)
        ? std::clamp((v - m_options.getMin()) / range, 0.f, 1.f)
        : 0.f;
    return m_options.getStartAngle() + t * sweepDegrees();
}

/*
    update(Rectf& parentBounds, float dt):
    - Params:   Rectf& parentBounds, float dt
    - Returns:  void
    - Desc:     Resolves the knob's bounds and tracks a vertical drag while one
                is running. Dragging up always turns the indicator clockwise,
                which means the value delta is flipped when the arc itself
                sweeps clockwise -- otherwise the same gesture would move the
                pointer in opposite directions on two knobs configured
                differently. The button state is polled so a release outside the
                window still ends the drag.
*/
void Knob::update(Rectf& parentBounds, float /*dt*/) {
    resize(parentBounds);

    if (m_dragging) {
        float mx, my;
        uint32_t btns = SDL_GetMouseState(&mx, &my);
        if (!(btns & SDL_BUTTON_MASK(SDL_BUTTON_LEFT))) {
            m_dragging = false;
        } else {
            if (m_uiloRef) m_uiloRef->requestCursor(CursorType::SizeVertical, 2);
            const float scale = m_uiloRef ? m_uiloRef->getScale() : 1.f;
            const float curY  = m_uiloRef ? m_uiloRef->getMousePosition().y : my;
            const float dy    = m_dragStartY - curY;   /* up = positive */
            const float range = m_options.getMax() - m_options.getMin();
            const float pxPerRange = std::max(1.f, m_options.getDragPixelsPerRange() * scale);
            /* Flip the delta when the arc sweeps clockwise, so dragging up
               always turns the indicator the same way. */
            const float sign = (sweepDegrees() >= 0.f) ? 1.f : -1.f;
            applyValue(m_dragStartVal + sign * (dy / pxPerRange) * range);
        }
    }
}

/*
    render():
    - Params:   none
    - Returns:  void
    - Desc:     Draws the body and its rim, the arc track with the filled
                portion over it, and the indicator. The body radius is half the
                smaller side less the room the arc and its gap need, so the ring
                sits around the body rather than spilling outside the element.
                Tessellation is adaptive -- roughly a segment per pixel of
                circumference, and no coarser than about half a degree on the
                arc -- so a large knob stays smooth without making a small one
                expensive, with the configured segment count acting as a floor.
*/
void Knob::render() {
    if (!m_modifier.getVisible()) { m_dirty = false; return; }
    if (!m_uiloRef) { m_dirty = false; return; }
    auto& renderer = m_uiloRef->getRenderer();
    const float scale = m_uiloRef->getScale();

    const float cx = m_bounds.position.x + m_bounds.size.x * 0.5f;
    const float cy = m_bounds.position.y + m_bounds.size.y * 0.5f;

    /* Half the smaller side, less the arc and its gap, so the ring sits around
       the body instead of outside the element. */
    const float arcThick = std::max(0.f, m_options.getArcThickness()) * scale;
    const float arcGap   = std::max(0.f, m_options.getArcGap())       * scale;
    const float outerR   = std::min(m_bounds.size.x, m_bounds.size.y) * 0.5f;
    const float bodyR    = std::max(1.f, outerR - arcThick - arcGap);
    const float arcR     = bodyR + arcGap + arcThick * 0.5f;

    /* Roughly one segment per pixel of circumference: smooth when large,
       cheap when small. */
    auto circleSegs = [](float r) {
        int s = (int)std::ceil(r * 1.2f);
        if (s < 64)  s = 64;
        if (s > 512) s = 512;
        return s;
    };

    /* Body fill and rim. */
    const Color bodyColor      = resolveColor(m_options.getBodyColorRole(),      m_options.getBodyColor());
    const Color outlineColor   = resolveColor(m_options.getOutlineColorRole(),   m_options.getOutlineColor());
    const Color trackColor     = resolveColor(m_options.getTrackColorRole(),     m_options.getTrackColor());
    const Color arcColor       = resolveColor(m_options.getArcColorRole(),       m_options.getArcColor());
    const Color indicatorColor = resolveColor(m_options.getIndicatorColorRole(), m_options.getIndicatorColor());

    if (m_options.getOutlineThickness() > 0.f && outlineColor.a > 0) {
        const float ot = m_options.getOutlineThickness() * scale;
        renderer.draw(Circle{{cx, cy}, bodyR + ot, circleSegs(bodyR + ot), outlineColor});
    }
    renderer.draw(Circle{{cx, cy}, bodyR, circleSegs(bodyR), bodyColor});

    /* Arc track, with the filled portion over it. */
    if (arcThick > 0.f) {
        const float sweep = sweepDegrees();
        const float sweepAbs = std::abs(sweep);

        /* At most about half a degree per segment, with the configured count
           as a floor. */
        const int userSegs    = std::max(8, m_options.getSegments());
        const int adaptive    = std::max(720, (int)std::ceil(arcR * 3.f));
        const int segsPerRev  = std::max(userSegs, adaptive);
        int trackSegs = (int)std::ceil(segsPerRev * (sweepAbs / 360.f));
        if (trackSegs < 2) trackSegs = 2;

        const float innerR = arcR - arcThick * 0.5f;
        const float outerR = arcR + arcThick * 0.5f;
        const float start  = m_options.getStartAngle();
        const float end    = start + sweep;

        renderer.drawArc({cx, cy}, innerR, outerR, start, end,
                         trackColor, trackSegs);

        const float curAngle  = angleForValue(m_value);
        const float fillSweep = curAngle - start;
        if (std::abs(fillSweep) > 0.001f) {
            int fillSegs = (sweepAbs > 0.f)
                ? (int)std::ceil(segsPerRev * (std::abs(fillSweep) / 360.f))
                : 0;
            if (fillSegs < 2) fillSegs = 2;
            renderer.drawArc({cx, cy}, innerR, outerR, start, curAngle,
                             arcColor, fillSegs);
        }
    }

    /* Indicator, running from inside the body out toward the rim. */
    if (m_options.getIndicatorThickness() > 0.f && indicatorColor.a > 0) {
        const float a = angleForValue(m_value) * kDeg2Rad;
        const float inset = std::clamp(m_options.getIndicatorInset(),  0.f, 1.f);
        const float len   = std::clamp(m_options.getIndicatorLength(), 0.f, 1.f);
        const float r0    = bodyR * inset;
        const float r1    = bodyR * len;
        Line ln;
        ln.start     = { cx + r0 * std::cos(a), cy + r0 * std::sin(a) };
        ln.end       = { cx + r1 * std::cos(a), cy + r1 * std::sin(a) };
        ln.thickness = m_options.getIndicatorThickness() * scale;
        ln.color     = indicatorColor;
        renderer.draw(ln);
    }

    m_dirty = false;
}

/*
    checkHover(const Vec2f& mousePosition):
    - Params:   const Vec2f& mousePosition
    - Returns:  bool -- true when the pointer is over the knob
    - Desc:     Asks for the vertical resize cursor while the pointer is inside,
                which is the axis a drag actually works on.
*/
bool Knob::checkHover(const Vec2f& mousePosition) {
    const float cx = m_bounds.position.x + m_bounds.size.x * 0.5f;
    const float cy = m_bounds.position.y + m_bounds.size.y * 0.5f;
    const float r  = std::min(m_bounds.size.x, m_bounds.size.y) * 0.5f;
    const float dx = mousePosition.x - cx;
    const float dy = mousePosition.y - cy;
    const bool inside = (dx*dx + dy*dy) <= r * r;
    if (inside && m_uiloRef) m_uiloRef->requestCursor(CursorType::SizeVertical, 1);
    m_hovered = inside;
    return inside;
}

/*
    checkLeftClick(const Vec2f& mousePosition):
    - Params:   const Vec2f& mousePosition
    - Returns:  bool -- true when the knob took the click
    - Desc:     Begins a drag from the current value, or restores the configured
                default on a double click. Unlike a Slider the value does not
                jump to the press: a knob has no position under the pointer to
                jump to.
*/
bool Knob::checkLeftClick(const Vec2f& mousePosition) {
    const float cx = m_bounds.position.x + m_bounds.size.x * 0.5f;
    const float cy = m_bounds.position.y + m_bounds.size.y * 0.5f;
    const float r  = std::min(m_bounds.size.x, m_bounds.size.y) * 0.5f;
    const float dx = mousePosition.x - cx;
    const float dy = mousePosition.y - cy;
    if ((dx*dx + dy*dy) > r * r) return false;

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

    m_dragging     = true;
    m_dragStartY   = mousePosition.y;
    m_dragStartVal = m_value;
    if (m_modifier.getOnLeftClick()) m_modifier.getOnLeftClick()(this);
    return true;
}

/*
    checkScroll(const Vec2f& mousePosition, float delta, bool precise, bool momentum):
    - Params:   const Vec2f& mousePosition, float delta, bool precise, bool
                momentum
    - Returns:  bool -- true when the knob consumed the event
    - Desc:     Adjusts the value by the wheel, accumulating the delta so a
                stepped knob still responds to deltas too small to cross an
                increment. Overscroll at either end is discarded rather than
                banked, so reversing direction moves the value immediately.
*/
bool Knob::checkScroll(
    const Vec2f& mousePosition,
    float delta,
    bool /*precise*/,
    bool /*momentum*/
) {
    const float cx = m_bounds.position.x + m_bounds.size.x * 0.5f;
    const float cy = m_bounds.position.y + m_bounds.size.y * 0.5f;
    const float r  = std::min(m_bounds.size.x, m_bounds.size.y) * 0.5f;
    const float dx = mousePosition.x - cx;
    const float dy = mousePosition.y - cy;
    if ((dx*dx + dy*dy) > r * r) return false;

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

    /* Drop overscroll at the boundaries rather than banking it. */
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
    - Desc:     Ends any drag when focus moves elsewhere.
*/
void Knob::onDeactivate() { m_dragging = false; }

/*
    setValue(float v):
    - Params:   float v
    - Returns:  void
    - Desc:     Sets the value programmatically, through the same clamping,
                snapping and change-callback path a drag uses.
*/
void Knob::setValue(float v) { applyValue(v); }

/*
    applyValue(float raw):
    - Params:   float raw
    - Returns:  void
    - Desc:     The single place the value changes: clamps into range, snaps to
                the step when one is set, and fires onValueChanged only when the
                result actually differs.
*/
void Knob::applyValue(float raw) {
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

} // namespace uilo
