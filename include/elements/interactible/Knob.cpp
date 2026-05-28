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

inline float wrap360(float a) {
    a = std::fmod(a, 360.f);
    if (a < 0.f) a += 360.f;
    return a;
}
} // namespace

Knob::Knob(Modifier modifier, KnobOptions options, const std::string& name)
    : m_options(options)
{
    m_modifier = modifier;
    m_name     = name;
    m_type     = ElementType::Knob;
    m_value    = m_options.hasDefault()
        ? std::clamp(m_options.getDefaultValue(), m_options.getMin(), m_options.getMax())
        : m_options.getMin();
}

float Knob::sweepDegrees() const {
    float diff = m_options.getEndAngle() - m_options.getStartAngle();
    if (m_options.getArcDirection() == KnobArcDir::CounterClockwise) {
        diff = wrap360(diff);
    } else {
        diff = -wrap360(-diff);
    }
    // Treat a zero/degenerate sweep as a full revolution so users get a
    // full ring instead of nothing when start == end.
    if (std::abs(diff) < 0.001f) {
        diff = (m_options.getArcDirection() == KnobArcDir::CounterClockwise) ? 360.f : -360.f;
    }
    return diff;
}

float Knob::angleForValue(float v) const {
    const float range = m_options.getMax() - m_options.getMin();
    const float t = (range > 0.f)
        ? std::clamp((v - m_options.getMin()) / range, 0.f, 1.f)
        : 0.f;
    return m_options.getStartAngle() + t * sweepDegrees();
}

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
            const float dy    = m_dragStartY - curY; // up = positive
            const float range = m_options.getMax() - m_options.getMin();
            const float pxPerRange = std::max(1.f, m_options.getDragPixelsPerRange() * scale);
            // Drag up = visually clockwise rotation, drag down = CCW.
            // In screen coords (+y down) "visually clockwise" means the
            // cartesian angle increases. Flip the value delta when the
            // arc sweeps clockwise (sweep<0) so the indicator still
            // rotates clockwise for an upward drag.
            const float sign = (sweepDegrees() >= 0.f) ? 1.f : -1.f;
            applyValue(m_dragStartVal + sign * (dy / pxPerRange) * range);
        }
    }
}

void Knob::render() {
    if (!m_uiloRef) { m_dirty = false; return; }
    auto& renderer = m_uiloRef->getRenderer();
    const float scale = m_uiloRef->getScale();

    const float cx = m_bounds.position.x + m_bounds.size.x * 0.5f;
    const float cy = m_bounds.position.y + m_bounds.size.y * 0.5f;

    // Outer radius = half the smaller side; reserve room for arc + gap so
    // the ring sits *around* the knob body without spilling out.
    const float arcThick = std::max(0.f, m_options.getArcThickness()) * scale;
    const float arcGap   = std::max(0.f, m_options.getArcGap())       * scale;
    const float outerR   = std::min(m_bounds.size.x, m_bounds.size.y) * 0.5f;
    const float bodyR    = std::max(1.f, outerR - arcThick - arcGap);
    const float arcR     = bodyR + arcGap + arcThick * 0.5f;

    // Adaptive tessellation so larger knobs don't show polygon edges.
    // ~1 segment per pixel of circumference keeps the silhouette smooth
    // up to large sizes while staying cheap on small ones.
    auto circleSegs = [](float r) {
        int s = (int)std::ceil(r * 1.2f);
        if (s < 64)  s = 64;
        if (s > 512) s = 512;
        return s;
    };

    // ---- Body fill + optional outline -----------------------------------
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

    // ---- Arc track + filled portion -------------------------------------
    if (arcThick > 0.f) {
        const float sweep = sweepDegrees();
        const float sweepAbs = std::abs(sweep);

        // Aim for <= ~0.5 degrees per segment so the strip stays smooth at
        // any size; honour the user's requested segments as a floor.
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

    // ---- Indicator (pointer from body interior out toward rim) ----------
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

bool Knob::checkLeftClick(const Vec2f& mousePosition) {
    const float cx = m_bounds.position.x + m_bounds.size.x * 0.5f;
    const float cy = m_bounds.position.y + m_bounds.size.y * 0.5f;
    const float r  = std::min(m_bounds.size.x, m_bounds.size.y) * 0.5f;
    const float dx = mousePosition.x - cx;
    const float dy = mousePosition.y - cy;
    if ((dx*dx + dy*dy) > r * r) return false;

    // Double-click within 350 ms snaps the knob back to its default.
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

bool Knob::checkScroll(const Vec2f& mousePosition, float delta) {
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
    applyValue(m_value + delta * step);
    if (m_modifier.getOnScroll()) m_modifier.getOnScroll()(this, delta);
    return true;
}

void Knob::onDeactivate() { m_dragging = false; }

void Knob::setValue(float v) { applyValue(v); }

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
