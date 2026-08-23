#pragma once

#include "Container.hpp"
#include "../../utils/Math.hpp"
#include <optional>
#include <unordered_map>
#include "../../utils/Theme.hpp"

#include "../../utils/Themed.hpp"

namespace uilo {

/*
    GridLineStyle:
    - Desc:     How the Canvas draws its grid behind the children. None draws
                nothing, Lines rules the full extent, Dots marks each
                intersection, and Crosses draws a small tick at each one.
*/
enum class GridLineStyle {
    None,
    Lines,
    Dots,
    Crosses,
};

/*
    CanvasOptions:
    - Desc:     Everything a Canvas draws and how it responds to panning and
                zooming: the backdrop, the grid metric children snap to, the
                grid's own appearance, the pan bounds, and the zoom range and
                locks. Colors come as a literal plus a role, where the role wins
                when it resolves against the active Palette. Sizes and
                coordinates are canvas-space pixels.
*/
class CanvasOptions {
public:
    UILO_THEMED(CanvasOptions)

    /*
        inheritFrom(const CanvasOptions& prototype):
        - Params:   const CanvasOptions& prototype
        - Returns:  void
        - Desc:     Fills in every field the call site left alone from the
                    theme's prototype, and leaves the rest exactly as it was
                    set. Run when the element binds to its UILO, and again
                    whenever that UILO's theme changes.
    */
    void inheritFrom(const CanvasOptions& prototype) {
        m_gridThickness.inherit(prototype.m_gridThickness);
        m_outlineThickness.inherit(prototype.m_outlineThickness);
        m_outerPadding.inherit(prototype.m_outerPadding);
        m_innerPadding.inherit(prototype.m_innerPadding);
        m_rounding.inherit(prototype.m_rounding);
        m_colorRole.inherit(prototype.m_colorRole);
        m_gradientRole.inherit(prototype.m_gradientRole);
        m_outlineColorRole.inherit(prototype.m_outlineColorRole);
        m_gridColorRole.inherit(prototype.m_gridColorRole);
    }

    // Space kept outside this element, inside the slot its parent gave it. It
    // shrinks the element rather than displacing a sibling. Unset follows the theme's default for this type.
    CanvasOptions& setOuterPadding(float px)   { m_outerPadding.set(px); return *this; }
    CanvasOptions& clearOuterPadding()         { m_outerPadding.clear(); return *this; }
    float  getOuterPadding()     const { return m_outerPadding.get().value_or(0.f); }

    // Space kept between this element's edge and the area its children are positioned in. Unset follows the theme's default for this type.
    CanvasOptions& setInnerPadding(float px)   { m_innerPadding.set(px); return *this; }
    CanvasOptions& clearInnerPadding()         { m_innerPadding.clear(); return *this; }
    float  getInnerPadding()     const { return m_innerPadding.get().value_or(0.f); }

    // Backdrop fill. A gradient takes precedence over color when active;
    // setGradientRole names a gradient stored in the Palette and wins over
    // the literal gradient when it resolves.
    CanvasOptions& setColor(const Color& c)             { m_color     = c; return *this; }
    CanvasOptions& setColorRole(const std::string& r)   { m_colorRole.set(r); return *this; }
    CanvasOptions& setGradient(const Gradient& g)        { m_gradient     = g; return *this; }
    CanvasOptions& setGradientRole(const std::string& r) { m_gradientRole.set(r); return *this; }
    CanvasOptions& setRounding(float r)                 { m_rounding.set(r); return *this; }

    // Outline -------------------------------------------------------------
    // A border drawn inside the element's bounds, so turning one on never
    // changes the space the element takes. Thickness and role default to
    // whatever Defaults.hpp gives a CanvasOptions -- 0 and transparent out of
    // the box, which is no border at all.
    CanvasOptions& setOutlineColor(const Color& c)           { m_outlineColor = c; return *this; }
    CanvasOptions& setOutlineColorRole(const std::string& r) { m_outlineColorRole.set(r); return *this; }
    CanvasOptions& setOutlineThickness(float px)             { m_outlineThickness.set(px); return *this; }

    // Grid metric. Children placed via addChild(elem, x, y) get x/y rounded
    // to the nearest multiple of these values. 0 on an axis disables snap
    // for that axis.
    CanvasOptions& setGridSize(float x, float y)        { m_gridSize  = {x, y}; return *this; }
    CanvasOptions& setGridSize(Vec2f s)                 { m_gridSize  = s; return *this; }

    // Grid backdrop visualization. Spacing is "every N grid cells" — 1
    // draws a marker at every cell, 4 every fourth cell, etc.
    CanvasOptions& setGridLineStyle(GridLineStyle s)    { m_gridStyle = s; return *this; }
    CanvasOptions& setGridLineColor(const Color& c)     { m_gridColor = c; return *this; }
    CanvasOptions& setGridLineColorRole(const std::string& r) { m_gridColorRole.set(r); return *this; }
    CanvasOptions& setGridLineThickness(float t)        { m_gridThickness.set(t); return *this; }
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
    const std::string& getColorRole()  const { return m_colorRole.get(); }
    const Gradient&    getGradient()     const { return m_gradient; }
    const std::string& getGradientRole() const { return m_gradientRole.get(); }
    float         getRounding()        const;
    Color              getOutlineColor()     const { return m_outlineColor; }
    const std::string& getOutlineColorRole() const { return m_outlineColorRole.get(); }
    float              getOutlineThickness() const { return m_outlineThickness.get(); }
    Vec2f         getGridSize()        const { return m_gridSize; }
    GridLineStyle getGridLineStyle()   const { return m_gridStyle; }
    Color         getGridLineColor()   const { return m_gridColor; }
    const std::string& getGridLineColorRole() const { return m_gridColorRole.get(); }
    float         getGridLineThickness() const { return m_gridThickness.get(); }
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
    Themed<std::optional<float>> m_outerPadding;
    Themed<std::optional<float>> m_innerPadding;
    Color       m_color         = Color{0, 0, 0, 0};
    Themed<std::string> m_colorRole;
    Gradient    m_gradient;
    Themed<std::string> m_gradientRole;
    Themed<std::optional<float>> m_rounding;
    Color                m_outlineColor = Color::Transparent;
    Themed<std::string> m_outlineColorRole;
    Themed<float> m_outlineThickness {0.f};

    Vec2f       m_gridSize      = {0.f, 0.f};
    GridLineStyle m_gridStyle   = GridLineStyle::None;
    Color       m_gridColor     = Color{255, 255, 255, 40};
    Themed<std::string> m_gridColorRole;
    Themed<float> m_gridThickness {1.f};
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
    // The theme role this was constructed with; resolved at bind time.
    std::string m_themeRole;
};

/*
    getRounding():
    - Params:   none
    - Returns:  float
    - Desc:     Corner radius, resolved in three steps: the value this element
                was given, then the active Theme's, then 0. Resolved on every
                read rather than cached, so changing the Theme restyles a canvas
                already on screen.
*/
inline float CanvasOptions::getRounding() const {
    return m_rounding.get().value_or(0.f);
}


/*
    Canvas:
    - Desc:     A Container that places its children at free canvas-space pixel
                coordinates inside a pannable, zoomable viewport, rather than
                flowing them along an axis the way Row and Column do. An
                optional grid metric snaps placement to a regular lattice, and
                optional per-side bounds clamp how far the view can travel. Pan
                comes from the trackpad or scroll wheel and, when enabled, a
                middle-mouse drag; zoom from a pinch or Ctrl-scroll, and can be
                locked per axis so a timeline can scale horizontally only.
    - Child positions live in a side table keyed by element rather than on the
      children themselves, so an ordinary element can be placed on a canvas
      without knowing anything about one.
*/
class Canvas : public Container {
public:
    void applyTheme(const Theme& theme) override {
        m_options.inheritFrom(theme.cascade<CanvasOptions>(m_options.getThemeRole()));
        Element::applyTheme(theme);
    }

    float getOuterPadding() const override { return m_options.getOuterPadding(); }
    float getInnerPadding() const override { return m_options.getInnerPadding(); }

    Canvas(
        Modifier modifier,
        CanvasOptions options,
        const std::string& name = ""
    );
    Canvas(
        Modifier modifier,
        CanvasOptions options,
        contains children,
        const std::string& name = ""
    );

    const CanvasOptions& getOptions() const { return m_options; }
    CanvasOptions&       getOptions()       { return m_options; }
    void                 setOptions(const CanvasOptions& o) { m_options = o; m_dirty = true; }

    // Places a child at canvas-space (x, y), snapped to the grid step on each
    // axis where that step is above 0.
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
    bool checkScroll(
        const Vec2f& mousePosition,
        float delta,
        bool precise = false,
        bool momentum = false
    ) override;
    bool checkScroll(
        const Vec2f& mousePosition,
        Vec2f delta,
        bool precise = false,
        bool momentum = false
    ) override;
    bool checkZoom(const Vec2f& mousePosition, float magnification) override;

private:
    Vec2f snap(Vec2f v) const;
    Vec2f clampPan(Vec2f pan) const;

    CanvasOptions m_options;
    std::unordered_map<Element*, Vec2f> m_positions;   /* child -> canvas-space pos */
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
