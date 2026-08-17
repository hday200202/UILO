#include "Canvas.hpp"

#include <SDL3/SDL.h>
#include <algorithm>
#include <cmath>
#include <vector>

#include "../../UILO.hpp"
#include "../../renderer/Shapes.hpp"
#include "../../utils/RenderUtils.hpp"

namespace uilo {

/*
    Canvas(Modifier modifier, CanvasOptions options, const std::string& name):
    - Params:   Modifier modifier, CanvasOptions options, const std::string&
                name
    - Returns:  Canvas
    - Desc:     Constructs an empty canvas. Children are added afterwards
                through addChild, which is what gives them a canvas-space
                position.
*/
Canvas::Canvas(
    Modifier modifier,
    CanvasOptions options,
    const std::string& name
) : Container(modifier, {}, name), m_options(options) {
    m_type = ElementType::Canvas;
}

/*
    Canvas(...):
    - Params:   Modifier modifier, CanvasOptions options, contains children,
                const std::string& name
    - Returns:  Canvas
    - Desc:     Constructs a canvas from a declaration list. Children given this
                way all land at canvas-space (0, 0), since the list carries no
                coordinates; move them afterwards with setChildPosition, or use
                addChild to place them as they are added.
*/
Canvas::Canvas(
    Modifier modifier,
    CanvasOptions options,
    contains children,
    const std::string& name
) : Container(modifier, children, name), m_options(options) {
    m_type = ElementType::Canvas;
    for (auto* c : children) m_positions[c] = {0.f, 0.f};
}

/*
    snap(Vec2f v):
    - Params:   Vec2f v
    - Returns:  Vec2f
    - Desc:     Rounds a canvas-space position to the nearest grid intersection.
                An axis whose grid step is 0 is left alone, so a canvas can snap
                on one axis and stay free on the other.
*/
Vec2f Canvas::snap(Vec2f v) const {
    const Vec2f g = m_options.getGridSize();
    if (g.x > 0.f) v.x = std::round(v.x / g.x) * g.x;
    if (g.y > 0.f) v.y = std::round(v.y / g.y) * g.y;
    return v;
}

/*
    clampPan(Vec2f pan):
    - Params:   Vec2f pan
    - Returns:  Vec2f
    - Desc:     Holds a pan offset inside the configured bounds. Each side is
                optional, so a canvas can be bounded on one edge and infinite on
                the others. The maximum is measured against the visible extent
                rather than the raw coordinate, so the far edge of the content
                stops at the far edge of the viewport instead of scrolling past
                it -- and because that extent depends on zoom, it is recomputed
                per axis here. When the bounded range is narrower than the
                viewport there is nothing to pan, so the offset pins to the
                minimum.
*/
Vec2f Canvas::clampPan(Vec2f pan) const {
    const auto& mnX = m_options.getMinX();
    const auto& mxX = m_options.getMaxX();
    const auto& mnY = m_options.getMinY();
    const auto& mxY = m_options.getMaxY();

    /* Visible extent in canvas-space units accounts for zoom (per axis). */
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

/*
    setZoom(float z):
    - Params:   float z
    - Returns:  void
    - Desc:     Sets both axes to the same zoom.
*/
void Canvas::setZoom(float z) {
    setZoom(z, z);
}

/*
    setZoom(float zx, float zy):
    - Params:   float zx, float zy
    - Returns:  void
    - Desc:     Sets each axis independently, clamped to the configured range.
                The pan is re-clamped afterwards, since changing zoom changes
                how much of the canvas is visible and so how far it may travel.
*/
void Canvas::setZoom(float zx, float zy) {
    const float cx = std::clamp(zx, m_options.getMinZoom(), m_options.getMaxZoom());
    const float cy = std::clamp(zy, m_options.getMinZoom(), m_options.getMaxZoom());
    if (cx == m_zoomX && cy == m_zoomY) return;
    m_zoomX = cx;
    m_zoomY = cy;
    m_pan   = clampPan(m_pan);
    m_dirty = true;
}

/*
    zoomAt(Vec2f pivotWindowPx, float factor):
    - Params:   Vec2f pivotWindowPx, float factor
    - Returns:  void
    - Desc:     Scales both axes by one factor about a pivot in window pixels,
                honouring the per-axis locks -- a locked axis is passed a factor
                of 1 and so is left where it is. Does nothing when zoom is
                disabled outright, or when both axes are locked.
*/
void Canvas::zoomAt(Vec2f pivotWindowPx, float factor) {
    if (!m_options.getZoomEnabled() || factor <= 0.f) return;
    const float fx = m_options.getZoomAxisX() ? factor : 1.f;
    const float fy = m_options.getZoomAxisY() ? factor : 1.f;
    if (fx == 1.f && fy == 1.f) return;
    zoomAt(pivotWindowPx, fx, fy);
}

/*
    zoomAt(Vec2f pivotWindowPx, float factorX, float factorY):
    - Params:   Vec2f pivotWindowPx, float factorX, float factorY
    - Returns:  void
    - Desc:     Scales each axis by its own factor about a pivot in window
                pixels, keeping the canvas point under that pivot visually
                fixed: the point is read at the old zoom and the pan rewritten
                so it lands back under the pivot at the new one. Ignores the
                axis locks, which is what makes it the programmatic entry point
                -- the gesture path goes through the single-factor overload.
*/
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

    /* Keep the canvas point under the pivot stationary in window space on each
       axis independently. */
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

/*
    addChild(Element* element, float x, float y):
    - Params:   Element* element, float x, float y
    - Returns:  void
    - Desc:     Adds a child and places it at a canvas-space position, snapped
                to the grid. Binds it to the owning UILO straight away when the
                canvas already has one, so an element added after the page is
                live is registered rather than left out of the element pool.
*/
void Canvas::addChild(Element* element, float x, float y) {
    if (!element) return;
    m_children.push_back(element);
    m_positions[element] = snap({x, y});
    if (m_uiloRef) element->setUILO(*m_uiloRef);
    m_dirty = true;
}

/*
    setChildPosition(Element* element, float x, float y):
    - Params:   Element* element, float x, float y
    - Returns:  void
    - Desc:     Moves an existing child to a canvas-space position, snapped to
                the grid.
*/
void Canvas::setChildPosition(Element* element, float x, float y) {
    if (!element) return;
    m_positions[element] = snap({x, y});
    m_dirty = true;
}

/*
    getChildPosition(Element* element):
    - Params:   Element* element
    - Returns:  Vec2f -- canvas-space position, origin when the element is
                unknown
    - Desc:     Where a child sits in canvas space.
*/
Vec2f Canvas::getChildPosition(Element* element) const {
    auto it = m_positions.find(element);
    return it == m_positions.end() ? Vec2f{0.f, 0.f} : it->second;
}

/*
    setPan(Vec2f pan):
    - Params:   Vec2f pan
    - Returns:  void
    - Desc:     Moves the viewport to an absolute canvas-space offset, clamped
                to the configured bounds. Only marks the canvas dirty when the
                offset actually moved, so driving this every frame from a
                handler costs nothing while the view is still.
*/
void Canvas::setPan(Vec2f pan) {
    Vec2f np = clampPan(pan);
    if (np.x != m_pan.x || np.y != m_pan.y) {
        m_pan = np;
        m_dirty = true;
    }
}

/*
    update(Rectf& parentBounds, float dt):
    - Params:   Rectf& parentBounds, float dt
    - Returns:  void
    - Desc:     Resolves the canvas's own bounds, services middle-mouse panning,
                then places every child at its canvas-space position transformed
                by the pan and zoom. Zoom is applied in two parts: a uniform
                boost to UILO's scale, so text, rounding and outlines grow with
                the view, and a per-axis size override applied after each child
                ticks, so a canvas zoomed differently on each axis stretches its
                children rather than only moving them. The scale boost is undone
                before returning, since it belongs to this subtree alone.
*/
void Canvas::update(Rectf& parentBounds, float dt) {
    /* Resolve modifier-driven size/align/padding so outerPadding, fixed
       width/height, and alignment work just like any other element. */
    resize(parentBounds);

    /* Middle-mouse drag-to-pan. Polled here because UILO has no middle-mouse
       plumbing of its own. */
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
            /* Drag in window pixels translates to drag in canvas units
               independently on each axis. */
            setPan({ m_dragPanStart.x - d.x / std::max(0.0001f, m_zoomX),
                     m_dragPanStart.y - d.y / std::max(0.0001f, m_zoomY) });
        }
    }

    /* Re-clamp the pan in case the viewport size changed. */
    m_pan = clampPan(m_pan);

    /* Uniform scale boost: the geometric mean of the two zoom axes, so text and
       rounding grow with the view. Per-axis stretch is applied after the tick. */
    const float baseScale = m_uiloRef ? m_uiloRef->getScale() : 1.f;
    const float geomZoom  = std::sqrt(std::max(0.0001f, m_zoomX * m_zoomY));
    if (m_uiloRef && geomZoom != 1.f) m_uiloRef->setScale(baseScale * geomZoom);

    for (auto* child : m_children) {
        if (!child) continue;
        const Vec2f canvasPos = getChildPosition(child);

        /* Size from the child's own declared dimension, so percent and pixel
           children both pick up the per-axis zoom. */
        const Dimension dw = child->getModifier().getWidth();
        const Dimension dh = child->getModifier().getHeight();
        /* Positions are relative to the content area, so inner padding insets
           the whole canvas surface rather than only its edges. */
        const Rectf area = contentArea();

        /* A canvas child has no siblings to leave anything over, so _flex on it can
           only mean the whole surface. */
        const float intrinsicW = dw.flex    ? area.size.x
                               : dw.percent ? (dw.value * 0.01f * area.size.x)
                                            : (dw.value * baseScale);
        const float intrinsicH = dh.flex    ? area.size.y
                               : dh.percent ? (dh.value * 0.01f * area.size.y)
                                            : (dh.value * baseScale);
        const float finalW = intrinsicW * m_zoomX;
        const float finalH = intrinsicH * m_zoomY;

        Rectf childBounds;
        childBounds.position = {
            area.position.x + (canvasPos.x - m_pan.x) * m_zoomX,
            area.position.y + (canvasPos.y - m_pan.y) * m_zoomY,
        };
        childBounds.size = { finalW, finalH };

        child->tick(childBounds, dt);

        /* Element::resize used the uniform boosted scale, so overwrite the
           bounds to lock in the per-axis size and ignore alignment shifts. */
        child->m_bounds.position = childBounds.position;
        child->m_bounds.size     = childBounds.size;
    }

    if (m_uiloRef && geomZoom != 1.f) m_uiloRef->setScale(baseScale);
}

/*
    render():
    - Params:   none
    - Returns:  void
    - Desc:     Draws the backdrop, the grid, and then the children, with
                everything after the backdrop clipped to the canvas viewport so
                content panned past an edge disappears cleanly. The grid is
                drawn in canvas space stepped by the grid metric, so it travels
                and scales with the content rather than being painted on the
                window.
*/
void Canvas::render() {
    if (!m_modifier.getVisible()) return;
    if (m_bounds.size.x <= 0.f || m_bounds.size.y <= 0.f) return;
    if (!m_uiloRef) return;

    auto& r = m_uiloRef->getRenderer();
    const float scale = m_uiloRef->getScale();

    /* Backdrop fill */
    const Color bg = m_uiloRef->getPalette().resolve(
        m_options.getColorRole(), m_options.getColor());
    const float rounding = m_options.getRounding() * scale;
    /* Border, resolved the same way the fill is; drawn inside the bounds. */
    const Color ol  = m_uiloRef->getPalette().resolve(
        m_options.getOutlineColorRole(), m_options.getOutlineColor());
    const float olT = m_options.getOutlineThickness() * scale;
    Color gc[4];
    if (resolveGradient(m_options.getGradient(), m_options.getGradientRole(), gc)) {
        if (rounding <= 0.f) {
            Rect shape{m_bounds.position, m_bounds.size, Color::White, ol, olT};
            shape.setGradientColors(gc);
            r.draw(shape);
        } else {
            RoundedRect shape{m_bounds.position, m_bounds.size, rounding, 8u,
                              Color::White, ol, olT};
            shape.setGradientColors(gc);
            r.draw(shape);
        }
    } else if (bg.a > 0 || olT > 0.f) {
        if (rounding <= 0.f)
            r.draw(Rect{m_bounds.position, m_bounds.size, bg, ol, olT});
        else
            r.draw(RoundedRect{m_bounds.position, m_bounds.size, rounding, 8u, bg, ol, olT});
    }

    /* Clip everything (grid + children) to the canvas viewport. */
    r.pushRoundClip(m_bounds, rounding);

    /* Grid backdrop */
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

        /* Canvas-space extents of the visible viewport (per axis). */
        const float x0 = m_pan.x;
        const float x1 = m_pan.x + m_bounds.size.x / zoomX;
        const float y0 = m_pan.y;
        const float y1 = m_pan.y + m_bounds.size.y / zoomY;

        /* First grid line >= x0, etc. */
        const float startX = std::ceil(x0 / stepX) * stepX;
        const float startY = std::ceil(y0 / stepY) * stepY;

        /* Optional canvas-bounds clipping for the grid drawing range. */
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

        /* Skip rendering when the grid step collapses below a few pixels on
           screen — would draw millions of markers and hurt fps. */
        const float minScreenStep = 4.f;
        const bool tooDense = (stepX * zoomX < minScreenStep) || (stepY * zoomY < minScreenStep);
        if (tooDense) {
            /* skip grid pass entirely */
        } else if (style == GridLineStyle::Lines) {
            std::vector<Line> lines;
            lines.reserve(static_cast<size_t>(approxX + approxY));
            /* Vertical lines. */
            for (float cx = startX; cx <= x1; cx += stepX) {
                if (cx < boundLoX || cx > boundHiX) continue;
                const float wx = toWinX(cx);
                lines.push_back(Line{
                    {wx, m_bounds.position.y},
                    {wx, m_bounds.position.y + m_bounds.size.y},
                    thick, gc
                });
            }
            /* Horizontal lines. */
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

    /* Children, drawn with the zoom pushed into UILO's scale so their rounding
       and text match the zoomed view. */
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

/*
    checkScroll(const Vec2f& mousePosition, float delta, bool precise, bool momentum):
    - Params:   const Vec2f& mousePosition, float delta, bool precise, bool
                momentum
    - Returns:  bool -- true when the canvas consumed the event
    - Desc:     Single-axis scroll, forwarded to the two-axis path as a vertical
                delta so a plain wheel pans the canvas down.
*/
bool Canvas::checkScroll(
    const Vec2f& mousePosition,
    float delta,
    bool precise,
    bool momentum
) {
    /* Single-axis fallback: treat as vertical scroll. */
    return checkScroll(mousePosition, Vec2f{0.f, delta}, precise, momentum);
}

/*
    checkScroll(const Vec2f& mousePosition, Vec2f delta, bool precise, bool momentum):
    - Params:   const Vec2f& mousePosition, Vec2f delta, bool precise, bool
                momentum
    - Returns:  bool -- true when the canvas consumed the event
    - Desc:     Two-axis scroll, offered to the children first so a scrollable
                element sitting on the canvas keeps its own gesture. Otherwise
                the delta pans the view, divided by the zoom so a drag covers
                the same on-screen distance whatever the view is scaled to.
*/
bool Canvas::checkScroll(
    const Vec2f& mousePosition,
    Vec2f delta,
    bool precise,
    bool momentum
) {
    if (!m_bounds.contains(mousePosition)) return false;

    /* Let children (e.g. a Column placed inside the canvas) consume the scroll
       first. */
    for (auto* child : m_children) {
        if (!child) continue;
        if (child->getBounds().contains(mousePosition))
            if (child->checkScroll(mousePosition, delta, precise, momentum)) return true;
    }

    /* Ctrl/Cmd + scroll = zoom at cursor. delta.y is treated as a signed zoom
       step. */
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

    /* Pan. The delta arrives in lines, so scrollSpeed converts it to pixels;
       dividing by zoom keeps a gesture the same distance on screen. */
    const float speed = m_options.getScrollSpeed();
    const float mulX  = speed / std::max(0.0001f, m_zoomX);
    const float mulY  = speed / std::max(0.0001f, m_zoomY);
    setPan({ m_pan.x - delta.x * mulX, m_pan.y - delta.y * mulY });

    if (m_modifier.getOnScroll()) m_modifier.getOnScroll()(this, delta.y);
    (void)momentum; (void)precise;
    return true;
}

/*
    checkZoom(const Vec2f& mousePosition, float magnification):
    - Params:   const Vec2f& mousePosition, float magnification
    - Returns:  bool -- true when the canvas consumed the gesture
    - Desc:     Pinch or Ctrl-scroll zoom about the pointer, offered to the
                children first so a nested zoomable element wins.
                `magnification` is an additive per-event ratio, so 0.05 means
                grow by five percent, and it is turned into a multiplier for
                zoomAt.
*/
bool Canvas::checkZoom(const Vec2f& mousePosition, float magnification) {
    if (!m_bounds.contains(mousePosition)) return false;
    if (!m_options.getZoomEnabled()) return false;
    /* Pinch: NSEvent.magnification is a per-event additive ratio. */
    zoomAt(mousePosition, 1.f + magnification);
    return true;
}

} // namespace uilo
