#include "Canvas.hpp"

#include <SDL3/SDL.h>
#include <algorithm>
#include <cmath>
#include <vector>

#include "../../UILO.hpp"
#include "../../renderer/Shapes.hpp"
#include "../../utils/RenderUtils.hpp"

namespace uilo {

Canvas::Canvas(Modifier modifier, CanvasOptions options, const std::string& name)
    : Container(modifier, {}, name)
    , m_options(options)
{
    m_type = ElementType::Canvas;
}

Canvas::Canvas(Modifier modifier, CanvasOptions options, contains children, const std::string& name)
    : Container(modifier, children, name)
    , m_options(options)
{
    m_type = ElementType::Canvas;
    // Children passed through the contains list land at (0,0) in
    // canvas-space; reposition them later with setChildPosition().
    for (auto* c : children) m_positions[c] = {0.f, 0.f};
}

Vec2f Canvas::snap(Vec2f v) const {
    const Vec2f g = m_options.getGridSize();
    if (g.x > 0.f) v.x = std::round(v.x / g.x) * g.x;
    if (g.y > 0.f) v.y = std::round(v.y / g.y) * g.y;
    return v;
}

Vec2f Canvas::clampPan(Vec2f pan) const {
    const auto& mnX = m_options.getMinX();
    const auto& mxX = m_options.getMaxX();
    const auto& mnY = m_options.getMinY();
    const auto& mxY = m_options.getMaxY();

    // Visible extent in canvas-space units accounts for zoom (per axis).
    const float vw = m_bounds.size.x / std::max(0.0001f, m_zoomX);
    const float vh = m_bounds.size.y / std::max(0.0001f, m_zoomY);

    if (mnX) pan.x = std::max(pan.x, *mnX);
    if (mxX) pan.x = std::min(pan.x, *mxX - vw);
    if (mnY) pan.y = std::max(pan.y, *mnY);
    if (mxY) pan.y = std::min(pan.y, *mxY - vh);

    if (mnX && mxX && (*mxX - *mnX) < vw) pan.x = *mnX;
    if (mnY && mxY && (*mxY - *mnY) < vh) pan.y = *mnY;
    return pan;
}

void Canvas::setZoom(float z) {
    setZoom(z, z);
}

void Canvas::setZoom(float zx, float zy) {
    const float cx = std::clamp(zx, m_options.getMinZoom(), m_options.getMaxZoom());
    const float cy = std::clamp(zy, m_options.getMinZoom(), m_options.getMaxZoom());
    if (cx == m_zoomX && cy == m_zoomY) return;
    m_zoomX = cx;
    m_zoomY = cy;
    m_pan   = clampPan(m_pan);
    m_dirty = true;
}

void Canvas::zoomAt(Vec2f pivotWindowPx, float factor) {
    if (!m_options.getZoomEnabled() || factor <= 0.f) return;
    const float fx = m_options.getZoomAxisX() ? factor : 1.f;
    const float fy = m_options.getZoomAxisY() ? factor : 1.f;
    if (fx == 1.f && fy == 1.f) return;
    zoomAt(pivotWindowPx, fx, fy);
}

void Canvas::zoomAt(Vec2f pivotWindowPx, float factorX, float factorY) {
    if (factorX <= 0.f || factorY <= 0.f) return;
    const float oldZX = m_zoomX;
    const float oldZY = m_zoomY;
    const float newZX = std::clamp(oldZX * factorX,
                                   m_options.getMinZoom(),
                                   m_options.getMaxZoom());
    const float newZY = std::clamp(oldZY * factorY,
                                   m_options.getMinZoom(),
                                   m_options.getMaxZoom());
    if (newZX == oldZX && newZY == oldZY) return;

    // Keep the canvas point under the pivot stationary in window space
    // on each axis independently.
    const Vec2f localPx{ pivotWindowPx.x - m_bounds.position.x,
                         pivotWindowPx.y - m_bounds.position.y };
    const Vec2f canvasPt{ m_pan.x + localPx.x / oldZX,
                          m_pan.y + localPx.y / oldZY };

    m_zoomX = newZX;
    m_zoomY = newZY;
    m_pan   = clampPan({ canvasPt.x - localPx.x / newZX,
                         canvasPt.y - localPx.y / newZY });
    m_dirty = true;
}

void Canvas::addChild(Element* element, float x, float y) {
    if (!element) return;
    m_children.push_back(element);
    m_positions[element] = snap({x, y});
    if (m_uiloRef) element->setUILO(*m_uiloRef);
    m_dirty = true;
}

void Canvas::setChildPosition(Element* element, float x, float y) {
    if (!element) return;
    m_positions[element] = snap({x, y});
    m_dirty = true;
}

Vec2f Canvas::getChildPosition(Element* element) const {
    auto it = m_positions.find(element);
    return it == m_positions.end() ? Vec2f{0.f, 0.f} : it->second;
}

void Canvas::setPan(Vec2f pan) {
    Vec2f np = clampPan(pan);
    if (np.x != m_pan.x || np.y != m_pan.y) {
        m_pan = np;
        m_dirty = true;
    }
}

void Canvas::update(Rectf& parentBounds, float dt) {
    // Resolve modifier-driven size/align/padding so outerPadding, fixed
    // width/height, and alignment work just like any other element.
    resize(parentBounds);

    // Middle-mouse drag-to-pan. Polled here because UILO has no
    // middle-mouse plumbing of its own.
    if (m_options.getMiddleMousePan()) {
        float mx = 0.f, my = 0.f;
        const Uint32 mask = SDL_GetMouseState(&mx, &my);
        const bool middleNow = (mask & SDL_BUTTON_MASK(SDL_BUTTON_MIDDLE)) != 0;
        const Vec2f mousePos{mx, my};
        const bool insideViewport = m_bounds.contains(mousePos);

        if (middleNow && !m_middleDown && insideViewport) {
            m_panActive      = true;
            m_dragMouseStart = mousePos;
            m_dragPanStart   = m_pan;
        }
        if (!middleNow) m_panActive = false;
        m_middleDown = middleNow;

        if (m_panActive) {
            const Vec2f d = mousePos - m_dragMouseStart;
            // Drag in window pixels translates to drag in canvas units
            // independently on each axis.
            setPan({ m_dragPanStart.x - d.x / std::max(0.0001f, m_zoomX),
                     m_dragPanStart.y - d.y / std::max(0.0001f, m_zoomY) });
        }
    }

    // Re-clamp the pan in case the viewport size changed.
    m_pan = clampPan(m_pan);

    // Push a uniform UILO scale boost = geometric mean of the per-axis
    // zoom. Drives text size, rounding, outlines, etc. so they grow
    // with the overall zoom level. Per-axis stretching of the child
    // bounding box is applied as a post-tick override below.
    const float baseScale = m_uiloRef ? m_uiloRef->getScale() : 1.f;
    const float geomZoom  = std::sqrt(std::max(0.0001f, m_zoomX * m_zoomY));
    if (m_uiloRef && geomZoom != 1.f) m_uiloRef->setScale(baseScale * geomZoom);

    for (auto* child : m_children) {
        if (!child) continue;
        const Vec2f canvasPos = getChildPosition(child);

        // Final per-axis size in window pixels. Compute up-front from
        // the child's declared dimension so percent and px both honor
        // independent zoom factors.
        const Dimension dw = child->getModifier().getWidth();
        const Dimension dh = child->getModifier().getHeight();
        const float intrinsicW = dw.percent ? (dw.value * 0.01f * m_bounds.size.x)
                                            : (dw.value * baseScale);
        const float intrinsicH = dh.percent ? (dh.value * 0.01f * m_bounds.size.y)
                                            : (dh.value * baseScale);
        const float finalW = intrinsicW * m_zoomX;
        const float finalH = intrinsicH * m_zoomY;

        Rectf childBounds;
        childBounds.position = {
            m_bounds.position.x + (canvasPos.x - m_pan.x) * m_zoomX,
            m_bounds.position.y + (canvasPos.y - m_pan.y) * m_zoomY,
        };
        childBounds.size = { finalW, finalH };

        child->tick(childBounds, dt);

        // Force the final bounds (Element::resize re-resolves using
        // the uniform geomZoom-pushed scale, which would size px
        // children to dim.value * baseScale * geomZoom and percent
        // children to pct * finalW; override here to lock in the
        // intended per-axis size and ignore any alignment shifts).
        child->m_bounds.position = childBounds.position;
        child->m_bounds.size     = childBounds.size;
    }

    if (m_uiloRef && geomZoom != 1.f) m_uiloRef->setScale(baseScale);
}

void Canvas::render() {
    if (!m_modifier.getVisible()) return;
    if (m_bounds.size.x <= 0.f || m_bounds.size.y <= 0.f) return;
    if (!m_uiloRef) return;

    auto& r = m_uiloRef->getRenderer();
    const float scale = m_uiloRef->getScale();

    // ---- Backdrop fill ---------------------------------------------------
    const Color bg = m_uiloRef->getPalette().resolve(
        m_options.getColorRole(), m_options.getColor());
    const float rounding = m_options.getRounding() * scale;
    if (bg.a > 0) {
        if (rounding <= 0.f)
            r.draw(Rect{m_bounds.position, m_bounds.size, bg});
        else
            r.draw(RoundedRect{m_bounds.position, m_bounds.size, rounding, 8u, bg});
    }

    // Clip everything (grid + children) to the canvas viewport.
    r.pushRoundClip(m_bounds, rounding);

    // ---- Grid backdrop ---------------------------------------------------
    const GridLineStyle style = m_options.getGridLineStyle();
    const Vec2f grid = m_options.getGridSize();
    if (style != GridLineStyle::None && grid.x > 0.f && grid.y > 0.f) {
        const Color gc = m_uiloRef->getPalette().resolve(
            m_options.getGridLineColorRole(), m_options.getGridLineColor());

        const int   spacing  = std::max(1, m_options.getGridLineSpacing());
        const float stepX    = grid.x * static_cast<float>(spacing);
        const float stepY    = grid.y * static_cast<float>(spacing);
        const float thick    = std::max(1.f, m_options.getGridLineThickness() * scale);
        const float cross    = m_options.getGridCrossSize() * scale;

        const float zoomX = std::max(0.0001f, m_zoomX);
        const float zoomY = std::max(0.0001f, m_zoomY);

        // Canvas-space extents of the visible viewport (per axis).
        const float x0 = m_pan.x;
        const float x1 = m_pan.x + m_bounds.size.x / zoomX;
        const float y0 = m_pan.y;
        const float y1 = m_pan.y + m_bounds.size.y / zoomY;

        // First grid line >= x0, etc.
        const float startX = std::ceil(x0 / stepX) * stepX;
        const float startY = std::ceil(y0 / stepY) * stepY;

        // Optional canvas-bounds clipping for the grid drawing range.
        const auto& mnX = m_options.getMinX();
        const auto& mxX = m_options.getMaxX();
        const auto& mnY = m_options.getMinY();
        const auto& mxY = m_options.getMaxY();
        const float boundLoX = mnX ? *mnX : -1e30f;
        const float boundHiX = mxX ? *mxX :  1e30f;
        const float boundLoY = mnY ? *mnY : -1e30f;
        const float boundHiY = mxY ? *mxY :  1e30f;

        auto toWinX = [&](float cx) { return m_bounds.position.x + (cx - m_pan.x) * zoomX; };
        auto toWinY = [&](float cy) { return m_bounds.position.y + (cy - m_pan.y) * zoomY; };

        auto countSamples = [](float start, float end, float step) -> uint64_t {
            if (step <= 0.f || end < start) return 0u;
            return static_cast<uint64_t>(std::floor((end - start) / step)) + 1u;
        };
        const uint64_t approxX = countSamples(startX, x1, stepX);
        const uint64_t approxY = countSamples(startY, y1, stepY);
        const uint64_t approxMarkers = approxX * approxY;
        constexpr uint64_t kMaxDenseMarkers = 12000u;
        const uint32_t lodStride = (approxMarkers > kMaxDenseMarkers)
            ? static_cast<uint32_t>(std::ceil(std::sqrt(
                  static_cast<double>(approxMarkers) /
                  static_cast<double>(kMaxDenseMarkers))))
            : 1u;

        // Skip rendering when the grid step collapses below a few pixels
        // on screen — would draw millions of markers and hurt fps.
        const float minScreenStep = 4.f;
        const bool tooDense = (stepX * zoomX < minScreenStep) || (stepY * zoomY < minScreenStep);
        if (tooDense) {
            // skip grid pass entirely
        } else if (style == GridLineStyle::Lines) {
            std::vector<Line> lines;
            lines.reserve(static_cast<size_t>(approxX + approxY));
            // Vertical lines.
            for (float cx = startX; cx <= x1; cx += stepX) {
                if (cx < boundLoX || cx > boundHiX) continue;
                const float wx = toWinX(cx);
                lines.push_back(Line{
                    {wx, m_bounds.position.y},
                    {wx, m_bounds.position.y + m_bounds.size.y},
                    thick, gc
                });
            }
            // Horizontal lines.
            for (float cy = startY; cy <= y1; cy += stepY) {
                if (cy < boundLoY || cy > boundHiY) continue;
                const float wy = toWinY(cy);
                lines.push_back(Line{
                    {m_bounds.position.x,                  wy},
                    {m_bounds.position.x + m_bounds.size.x, wy},
                    thick, gc
                });
            }
            if (!lines.empty()) r.drawLines(lines.data(), lines.size());
        } else if (style == GridLineStyle::Dots) {
            const float radius = std::max(1.f, thick);
            uint32_t yi = 0u;
            for (float cy = startY; cy <= y1; cy += stepY, ++yi) {
                if (cy < boundLoY || cy > boundHiY) continue;
                if ((yi % lodStride) != 0u) continue;
                uint32_t xi = 0u;
                for (float cx = startX; cx <= x1; cx += stepX, ++xi) {
                    if (cx < boundLoX || cx > boundHiX) continue;
                    if ((xi % lodStride) != 0u) continue;
                    r.draw(Circle{{toWinX(cx), toWinY(cy)}, radius, 12, gc});
                }
            }
        } else if (style == GridLineStyle::Crosses) {
            const float halfArm = std::max(2.f, cross * 0.5f);
            std::vector<Line> lines;
            lines.reserve(static_cast<size_t>((approxMarkers / (lodStride * lodStride)) * 2u + 8u));
            uint32_t yi = 0u;
            for (float cy = startY; cy <= y1; cy += stepY, ++yi) {
                if (cy < boundLoY || cy > boundHiY) continue;
                if ((yi % lodStride) != 0u) continue;
                uint32_t xi = 0u;
                for (float cx = startX; cx <= x1; cx += stepX, ++xi) {
                    if (cx < boundLoX || cx > boundHiX) continue;
                    if ((xi % lodStride) != 0u) continue;
                    const float wx = toWinX(cx);
                    const float wy = toWinY(cy);
                    lines.push_back(Line{{wx - halfArm, wy}, {wx + halfArm, wy}, thick, gc});
                    lines.push_back(Line{{wx, wy - halfArm}, {wx, wy + halfArm}, thick, gc});
                }
            }
            if (!lines.empty()) r.drawLines(lines.data(), lines.size());
        }
    }

    // ---- Children --------------------------------------------------------
    // Push canvas zoom into UILO scale for the duration of child render
    // so rounding / outline / text size / etc. all scale with zoom.
    // Use the geometric mean of the two axes; per-axis stretch is
    // already baked into each child's bounds in update().
    const float oldScale = m_uiloRef->getScale();
    const float geomZoom = std::sqrt(std::max(0.0001f, m_zoomX * m_zoomY));
    if (geomZoom != 1.f) m_uiloRef->setScale(oldScale * geomZoom);

    for (auto* child : m_children) {
        if (!child) continue;
        if (child->getType() == ElementType::Resizer) continue;
        child->render();
    }

    if (geomZoom != 1.f) m_uiloRef->setScale(oldScale);

    r.popRoundClip();

    m_dirty = false;
}

bool Canvas::checkScroll(const Vec2f& mousePosition, float delta, bool precise, bool momentum) {
    // Single-axis fallback: treat as vertical scroll.
    return checkScroll(mousePosition, Vec2f{0.f, delta}, precise, momentum);
}

bool Canvas::checkScroll(const Vec2f& mousePosition, Vec2f delta, bool precise, bool momentum) {
    if (!m_bounds.contains(mousePosition)) return false;

    // Let children (e.g. a Column placed inside the canvas) consume the
    // scroll first.
    for (auto* child : m_children) {
        if (!child) continue;
        if (child->getBounds().contains(mousePosition))
            if (child->checkScroll(mousePosition, delta, precise, momentum)) return true;
    }

    // Ctrl/Cmd + scroll = zoom at cursor. delta.y is treated as a
    // signed zoom step.
    if (m_options.getZoomEnabled()) {
        const SDL_Keymod mods = SDL_GetModState();
        const bool ctrl = (mods & (SDL_KMOD_CTRL | SDL_KMOD_GUI)) != 0;
        if (ctrl && delta.y != 0.f) {
            const float factor = 1.f + delta.y * m_options.getZoomStep();
            zoomAt(mousePosition, factor);
            (void)momentum;
            return true;
        }
    }

    // Pan. scrollSpeed applies always (the mac monitor passes "lines"
    // i.e. pixels/30, so scrollSpeed ~30..60 gives a comfortable feel).
    // Divide by zoom (per axis) so the screen-pixel pan rate stays
    // constant as the user zooms in.
    const float speed = m_options.getScrollSpeed();
    const float mulX  = speed / std::max(0.0001f, m_zoomX);
    const float mulY  = speed / std::max(0.0001f, m_zoomY);
    setPan({ m_pan.x - delta.x * mulX, m_pan.y - delta.y * mulY });

    if (m_modifier.getOnScroll()) m_modifier.getOnScroll()(this, delta.y);
    (void)momentum; (void)precise;
    return true;
}

bool Canvas::checkZoom(const Vec2f& mousePosition, float magnification) {
    if (!m_bounds.contains(mousePosition)) return false;
    if (!m_options.getZoomEnabled()) return false;
    // Pinch: NSEvent.magnification is a per-event additive ratio.
    zoomAt(mousePosition, 1.f + magnification);
    return true;
}

} // namespace uilo
