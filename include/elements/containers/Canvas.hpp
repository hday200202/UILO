#pragma once

#include "Container.hpp"
#include "../../utils/Math.hpp"
#include <optional>
#include <unordered_map>

namespace uilo {

enum class GridLineStyle {
    None,
    Lines,
    Dots,
    Crosses,
};

class CanvasOptions {
public:
    CanvasOptions() = default;

    // Backdrop fill. A gradient takes precedence over color when active;
    // setGradientRole names a gradient stored in the Palette and wins over
    // the literal gradient when it resolves.
    CanvasOptions& setColor(const Color& c)             { m_color     = c; return *this; }
    CanvasOptions& setColorRole(const std::string& r)   { m_colorRole = r; return *this; }
    CanvasOptions& setGradient(const Gradient& g)        { m_gradient     = g; return *this; }
    CanvasOptions& setGradientRole(const std::string& r) { m_gradientRole = r; return *this; }
    CanvasOptions& setRounding(float r)                 { m_rounding  = r; return *this; }

    // Grid metric. Children placed via addChild(elem, x, y) get x/y rounded
    // to the nearest multiple of these values. 0 on an axis disables snap
    // for that axis.
    CanvasOptions& setGridSize(float x, float y)        { m_gridSize  = {x, y}; return *this; }
    CanvasOptions& setGridSize(Vec2f s)                 { m_gridSize  = s; return *this; }

    // Grid backdrop visualization. Spacing is "every N grid cells" — 1
    // draws a marker at every cell, 4 every fourth cell, etc.
    CanvasOptions& setGridLineStyle(GridLineStyle s)    { m_gridStyle = s; return *this; }
    CanvasOptions& setGridLineColor(const Color& c)     { m_gridColor = c; return *this; }
    CanvasOptions& setGridLineColorRole(const std::string& r) { m_gridColorRole = r; return *this; }
    CanvasOptions& setGridLineThickness(float t)        { m_gridThickness = t; return *this; }
    CanvasOptions& setGridLineSpacing(int n)            { m_gridSpacing = n < 1 ? 1 : n; return *this; }
    CanvasOptions& setGridCrossSize(float px)           { m_gridCrossSize = px; return *this; }

    // Pan bounds, in canvas-space pixels. Unset = infinite on that side.
    // minX set + maxX unset = can pan from minX rightward forever.
    CanvasOptions& setMinX(float v) { m_minX = v; return *this; }
    CanvasOptions& setMaxX(float v) { m_maxX = v; return *this; }
    CanvasOptions& setMinY(float v) { m_minY = v; return *this; }
    CanvasOptions& setMaxY(float v) { m_maxY = v; return *this; }
    CanvasOptions& clearMinX()      { m_minX.reset(); return *this; }
    CanvasOptions& clearMaxX()      { m_maxX.reset(); return *this; }
    CanvasOptions& clearMinY()      { m_minY.reset(); return *this; }
    CanvasOptions& clearMaxY()      { m_maxY.reset(); return *this; }

    // Multiplier on scroll-wheel deltas when panning. 1.0 = one grid cell
    // per wheel notch with default gridSize.
    CanvasOptions& setScrollSpeed(float s)              { m_scrollSpeed = s; return *this; }

    // Enable middle-mouse drag-to-pan. On by default.
    CanvasOptions& setMiddleMousePan(bool v)            { m_middlePan = v; return *this; }

    // Zoom range. Default 0.1 .. 10.
    CanvasOptions& setMinZoom(float v)                  { m_minZoom = v; return *this; }
    CanvasOptions& setMaxZoom(float v)                  { m_maxZoom = v; return *this; }
    // Per-event zoom factor for Ctrl+scroll (delta.y of 1.0 = grow this
    // fraction). Default 0.1 (10% per notch). Pinch magnification is
    // applied directly, not scaled by this.
    CanvasOptions& setZoomStep(float v)                 { m_zoomStep = v; return *this; }
    // Disable zoom entirely (Ctrl+scroll and pinch both no-op).
    CanvasOptions& setZoomEnabled(bool v)               { m_zoomEnabled = v; return *this; }
    // Per-axis zoom locks. When an axis is locked, zoom inputs
    // (pinch / Ctrl+scroll) leave that axis at its current value.
    // Default: both axes free.
    CanvasOptions& setZoomAxes(bool x, bool y)          { m_zoomAxisX = x; m_zoomAxisY = y; return *this; }
    CanvasOptions& setZoomAxisX(bool v)                 { m_zoomAxisX = v; return *this; }
    CanvasOptions& setZoomAxisY(bool v)                 { m_zoomAxisY = v; return *this; }

    Color         getColor()           const { return m_color; }
    const std::string& getColorRole()  const { return m_colorRole; }
    const Gradient&    getGradient()     const { return m_gradient; }
    const std::string& getGradientRole() const { return m_gradientRole; }
    float         getRounding()        const { return m_rounding; }
    Vec2f         getGridSize()        const { return m_gridSize; }
    GridLineStyle getGridLineStyle()   const { return m_gridStyle; }
    Color         getGridLineColor()   const { return m_gridColor; }
    const std::string& getGridLineColorRole() const { return m_gridColorRole; }
    float         getGridLineThickness() const { return m_gridThickness; }
    int           getGridLineSpacing() const { return m_gridSpacing; }
    float         getGridCrossSize()   const { return m_gridCrossSize; }
    const std::optional<float>& getMinX() const { return m_minX; }
    const std::optional<float>& getMaxX() const { return m_maxX; }
    const std::optional<float>& getMinY() const { return m_minY; }
    const std::optional<float>& getMaxY() const { return m_maxY; }
    float         getScrollSpeed()     const { return m_scrollSpeed; }
    bool          getMiddleMousePan()  const { return m_middlePan; }
    float         getMinZoom()         const { return m_minZoom; }
    float         getMaxZoom()         const { return m_maxZoom; }
    float         getZoomStep()        const { return m_zoomStep; }
    bool          getZoomEnabled()     const { return m_zoomEnabled; }
    bool          getZoomAxisX()       const { return m_zoomAxisX; }
    bool          getZoomAxisY()       const { return m_zoomAxisY; }

private:
    Color       m_color         = Color{0, 0, 0, 0};
    std::string m_colorRole;
    Gradient    m_gradient;
    std::string m_gradientRole;
    float       m_rounding      = 0.f;

    Vec2f       m_gridSize      = {0.f, 0.f};
    GridLineStyle m_gridStyle   = GridLineStyle::None;
    Color       m_gridColor     = Color{255, 255, 255, 40};
    std::string m_gridColorRole;
    float       m_gridThickness = 1.f;
    int         m_gridSpacing   = 1;
    float       m_gridCrossSize = 6.f;

    std::optional<float> m_minX, m_maxX, m_minY, m_maxY;

    float       m_scrollSpeed   = 40.f;
    bool        m_middlePan     = true;
    float       m_minZoom       = 0.1f;
    float       m_maxZoom       = 10.f;
    float       m_zoomStep      = 0.1f;
    bool        m_zoomEnabled   = true;
    bool        m_zoomAxisX     = true;
    bool        m_zoomAxisY     = true;
};

// Canvas: a Container that places children at free canvas-space pixel
// coordinates inside a pannable viewport. Optional grid metric snaps
// placement positions to a regular lattice; optional bounds clamp the
// pan extent. Pan input comes from the trackpad / scroll wheel and
// (when enabled) middle-mouse drag.
class Canvas : public Container {
public:
    Canvas(Modifier modifier, CanvasOptions options, const std::string& name = "");
    Canvas(Modifier modifier, CanvasOptions options, contains children, const std::string& name = "");

    const CanvasOptions& getOptions() const { return m_options; }
    CanvasOptions&       getOptions()       { return m_options; }
    void setOptions(const CanvasOptions& o) { m_options = o; m_dirty = true; }

    // Place a child at canvas-space (x, y). The position is snapped to
    // the grid step on each axis where the step is > 0.
    void addChild(Element* element, float x, float y);
    void setChildPosition(Element* element, float x, float y);
    Vec2f getChildPosition(Element* element) const;

    Vec2f getPan() const { return m_pan; }
    void  setPan(Vec2f pan);
    void  panBy(Vec2f delta) { setPan(m_pan + delta); }

    float getZoom()  const { return 0.5f * (m_zoomX + m_zoomY); }
    float getZoomX() const { return m_zoomX; }
    float getZoomY() const { return m_zoomY; }
    // Set both axes to the same zoom.
    void  setZoom(float z);
    // Set each axis independently. Values outside [minZoom, maxZoom] are clamped.
    void  setZoom(float zx, float zy);
    // Scale by `factor` keeping the canvas point under `pivotWindowPx`
    // visually fixed. pivot is in window pixels (the same space as
    // m_bounds). Honors the per-axis zoom locks: a locked axis is left
    // unchanged.
    void  zoomAt(Vec2f pivotWindowPx, float factor);
    // Per-axis explicit zoomAt. Ignores axis locks (programmatic use).
    void  zoomAt(Vec2f pivotWindowPx, float factorX, float factorY);

    void update(Rectf& parentBounds, float dt) override;
    void render() override;
    bool checkScroll(const Vec2f& mousePosition, float delta, bool precise = false, bool momentum = false) override;
    bool checkScroll(const Vec2f& mousePosition, Vec2f delta, bool precise = false, bool momentum = false) override;
    bool checkZoom(const Vec2f& mousePosition, float magnification) override;

private:
    Vec2f snap(Vec2f v) const;
    Vec2f clampPan(Vec2f pan) const;

    CanvasOptions m_options;
    std::unordered_map<Element*, Vec2f> m_positions; // child -> canvas-space pos
    Vec2f m_pan   = {0.f, 0.f};
    float m_zoomX = 1.f;
    float m_zoomY = 1.f;

    // Middle-mouse drag-to-pan state.
    bool  m_middleDown    = false;
    bool  m_panActive     = false;
    Vec2f m_dragMouseStart{};
    Vec2f m_dragPanStart{};
};

} // namespace uilo
