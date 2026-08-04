#pragma once

#include <optional>

#include "Container.hpp"
#include "../../utils/Theme.hpp"

namespace uilo {

/*
    RowOptions:
    - Desc:     Everything a Row draws that is not layout: background fill,
                gradient, corner rounding, border, scrolling, the subdivision
                grid and zoom. Colors come as a literal plus a role, where the
                role wins when it resolves against the active Palette and the
                literal is the fallback. A gradient takes precedence over a flat
                fill when active, and a Material on the Modifier takes
                precedence over both. Sizes are unscaled content pixels; UILO
                multiplies by its scale at layout time.
*/
class RowOptions {
public:
    RowOptions() = default;

    // Space kept outside this element, inside the slot its parent gave it. It
    // shrinks the element rather than displacing a sibling. Unset follows
    // Theme::setOuterPadding().
    RowOptions& setOuterPadding(float px)   { m_outerPadding = px; return *this; }
    RowOptions& clearOuterPadding()         { m_outerPadding.reset(); return *this; }
    float  getOuterPadding()     const { return Theme::resolveOuterPadding(m_outerPadding, 0.f); }

    // Space kept between this element's edge and the area its children are laid out in. Unset follows
    // Theme::setInnerPadding().
    RowOptions& setInnerPadding(float px)   { m_innerPadding = px; return *this; }
    RowOptions& clearInnerPadding()         { m_innerPadding.reset(); return *this; }
    float  getInnerPadding()     const { return Theme::resolveInnerPadding(m_innerPadding, 0.f); }

    // Background
    RowOptions& setColor(const Color& c)              { m_color = c; return *this; }
    RowOptions& setColorRole(const std::string& r)    { m_colorRole = r; return *this; }
    RowOptions& setGradient(const Gradient& g)        { m_gradient = g; return *this; }
    RowOptions& setGradientRole(const std::string& r) { m_gradientRole = r; return *this; }
    RowOptions& setRounding(float r)                  { m_rounding = r; return *this; }
    RowOptions& inheritRounding(const std::optional<float>& own, float fallback);

    // Outline: a border drawn inside the bounds, so enabling one never changes
    // the space the element takes.
    RowOptions& setOutlineColor(const Color& c)           { m_outlineColor = c; return *this; }
    RowOptions& setOutlineColorRole(const std::string& r) { m_outlineColorRole = r; return *this; }
    RowOptions& setOutlineThickness(float px)             { m_outlineThickness = px; return *this; }

    // Scrolling
    RowOptions& setScrollable(bool v)                { m_scrollable = v; return *this; }
    RowOptions& setScrollSpeed(float s)              { m_scrollSpeed = s; return *this; }
    RowOptions& setScrollMin(float v)                { m_scrollMin = v; m_scrollMinSet = true; return *this; }
    RowOptions& setScrollMax(float v)                { m_scrollMax = v; m_scrollMaxSet = true; return *this; }
    RowOptions& setScrollLink(const std::string& id) { m_scrollLink = id; return *this; }

    // Subdivision grid: setSubDivisions is the spacing between primary lines,
    // and major/minor define a repeated hierarchy within each division -- 1
    // major and 3 minor gives four equal slices. Lines are positioned relative
    // to the scroll offset, so they travel with the content.
    RowOptions& setSubDivisions(float px)                     { m_subDivisions = px; return *this; }
    RowOptions& setSubDivisionMajor(unsigned int count)       { m_subDivMajor = count; return *this; }
    RowOptions& setSubDivisionMinor(unsigned int count)       { m_subDivMinor = count; return *this; }
    RowOptions& setSubDivisionColor(Color c)                  { m_subDivColor = c; return *this; }
    RowOptions& setSubDivisionColorRole(const std::string& r) { m_subDivColorRole = r; return *this; }
    RowOptions& setSubDivisionMinScreenPx(float px)           { m_subDivMinPx = px; return *this; }
    RowOptions& setSubDivisionResubdivideMinScreenPx(float px) { m_subDivResubdivideMinPx = px; return *this; }
    RowOptions& setSubDivisionsMinDistance(float px)          { m_subDivMinDistance = px; return *this; }
    RowOptions& setSubDivisionsMaxDistance(float px)          { m_subDivMaxDistance = px; return *this; }
    RowOptions& setSubDivisionStripeEvery(unsigned int count) { m_subDivStripeEvery = count; return *this; }
    RowOptions& setSubDivisionStripeColor(Color c)            { m_subDivStripeColor = c; return *this; }
    RowOptions& setSubDivisionStripeColorRole(const std::string& r) { m_subDivStripeColorRole = r; return *this; }

    // Zoom: scales content widths and the grid under pinch or scroll-zoom.
    RowOptions& setZoomableX(bool v)               { m_zoomableX = v; return *this; }
    RowOptions& setZoomMin(float v)                { m_zoomMin = v; return *this; }
    RowOptions& setZoomMax(float v)                { m_zoomMax = v; return *this; }
    RowOptions& setZoomLink(const std::string& id) { m_zoomLink = id; return *this; }

    Color              getColor()        const { return m_color; }
    const std::string& getColorRole()    const { return m_colorRole; }
    const Gradient&    getGradient()     const { return m_gradient; }
    const std::string& getGradientRole() const { return m_gradientRole; }
    float              getRounding()     const;
    const std::optional<float>& getRoundingOpt()      const { return m_rounding; }
    float                       getRoundingFallback() const { return m_roundingFallback; }

    Color              getOutlineColor()     const { return m_outlineColor; }
    const std::string& getOutlineColorRole() const { return m_outlineColorRole; }
    float              getOutlineThickness() const { return m_outlineThickness; }

    bool               getScrollable()  const { return m_scrollable; }
    float              getScrollSpeed() const { return m_scrollSpeed; }
    float              getScrollMin()   const { return m_scrollMin; }
    float              getScrollMax()   const { return m_scrollMax; }
    bool               hasScrollMin()   const { return m_scrollMinSet; }
    bool               hasScrollMax()   const { return m_scrollMaxSet; }
    const std::string& getScrollLink()  const { return m_scrollLink; }

    float              getSubDivisions()           const { return m_subDivisions; }
    unsigned int       getSubDivisionMajor()       const { return m_subDivMajor; }
    unsigned int       getSubDivisionMinor()       const { return m_subDivMinor; }
    Color              getSubDivisionColor()       const { return m_subDivColor; }
    const std::string& getSubDivisionColorRole()   const { return m_subDivColorRole; }
    float              getSubDivisionMinScreenPx() const { return m_subDivMinPx; }
    float              getSubDivisionResubdivideMinScreenPx() const { return m_subDivResubdivideMinPx; }
    float              getSubDivisionsMinDistance() const { return m_subDivMinDistance; }
    float              getSubDivisionsMaxDistance() const { return m_subDivMaxDistance; }
    unsigned int       getSubDivisionStripeEvery() const { return m_subDivStripeEvery; }
    Color              getSubDivisionStripeColor() const { return m_subDivStripeColor; }
    const std::string& getSubDivisionStripeColorRole() const { return m_subDivStripeColorRole; }

    bool               getZoomableX() const { return m_zoomableX; }
    float              getZoomMin()   const { return m_zoomMin; }
    float              getZoomMax()   const { return m_zoomMax; }
    const std::string& getZoomLink()  const { return m_zoomLink; }

private:
    std::optional<float> m_outerPadding;
    std::optional<float> m_innerPadding;
    Color       m_color = Color{0, 0, 0, 0};
    std::string m_colorRole;
    Gradient    m_gradient;
    std::string m_gradientRole;

    std::optional<float> m_rounding;
    float                m_roundingFallback = 0.f;

    Color       m_outlineColor = Color::Transparent;
    std::string m_outlineColorRole;
    float       m_outlineThickness = 0.f;

    bool        m_scrollable   = false;
    float       m_scrollSpeed  = 40.f;
    float       m_scrollMin    = 0.f;
    float       m_scrollMax    = 0.f;
    bool        m_scrollMinSet = false;
    bool        m_scrollMaxSet = false;
    std::string m_scrollLink;

    float        m_subDivisions           = 0.f;
    unsigned int m_subDivMajor            = 1;
    unsigned int m_subDivMinor            = 3;
    Color        m_subDivColor            = Color{255, 255, 255, 30};
    std::string  m_subDivColorRole;
    float        m_subDivMinPx            = 4.f;
    float        m_subDivResubdivideMinPx = 24.f;
    float        m_subDivMinDistance      = 0.f;
    float        m_subDivMaxDistance      = 0.f;
    unsigned int m_subDivStripeEvery      = 0;
    Color        m_subDivStripeColor      = Color{0, 0, 0, 0};
    std::string  m_subDivStripeColorRole;

    bool        m_zoomableX = false;
    float       m_zoomMin   = 0.1f;
    float       m_zoomMax   = 50.f;
    std::string m_zoomLink;
};


/*
    inheritRounding(const std::optional<float>& own, float fallback):
    - Params:   const std::optional<float>& own, float fallback
    - Returns:  RowOptions&
    - Desc:     Takes a rounding straight from a composite widget. `own` is
                whatever the widget was told, empty when nothing, and `fallback`
                is that widget's own default for when the theme is silent too.
                Passing it through unresolved, rather than as a number the
                widget already worked out, is what lets the theme keep reaching
                this element after it has been built.
*/
inline RowOptions& RowOptions::inheritRounding(
    const std::optional<float>& own,
    float fallback
) {
    m_rounding         = own;
    m_roundingFallback = fallback;
    return *this;
}


/*
    getRounding():
    - Params:   none
    - Returns:  float
    - Desc:     Corner radius, resolved in three steps: the value this element
                was given, then the active Theme's, then the fallback carried by
                inheritRounding (0 for a plain Row). Resolved on every read
                rather than cached, so changing the Theme restyles an element
                already on screen.
*/
inline float RowOptions::getRounding() const {
    return Theme::resolveRounding(m_rounding, m_roundingFallback);
}


/*
    Row:
    - Desc:     A Container that lays its children out left to right,
                distributing the horizontal axis between them and giving each
                the full height. Optionally scrolls, zooms, and draws a
                subdivision grid behind its children. Column is the same class
                with the axes exchanged.
*/
class Row : public Container {
public:
    float getOuterPadding() const override { return m_options.getOuterPadding(); }
    float getInnerPadding() const override { return m_options.getInnerPadding(); }

    using Container::Container;

    explicit Row(
        Modifier modifier,
        RowOptions options,
        contains children,
        const std::string& name = ""
    );

    const RowOptions& getOptions() const { return m_options; }
    RowOptions&       getOptions()       { return m_options; }
    void              setOptions(const RowOptions& opts);

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

    float getZoomX() const { return m_zoomX; }
    void  setZoomX(float z);

private:
    // Layout, one phase per function. update() resolves its own bounds and then
    // runs exactly one of the two paths, followed by the two that apply to both.
    void updateScrollable(float dt, float scale);
    void updateFlow(float dt, float scale);
    void layoutResizers(float dt, float scale);
    void tickHiddenChildren(float dt);

    // Drawing, in the order render() calls them.
    void renderBackground(float scale);
    void renderSubdivisions(float scale);
    void renderChildren();
    void renderChildPass(bool ignoreScrollChildren, bool viewportInset);

    // Scroll state.
    float contentOverflow() const;
    float scrollStep(bool precise) const;
    void  readScrollLinks();
    void  publishScrollLinks() const;
    void  applyScrollOffset(float target);

    RowOptions m_options;
    float      m_scrollOffset        = 0.f;
    float      m_contentWidth        = 0.f;
    float      m_scrollViewportWidth = 0.f;
    float      m_scrollViewportX     = 0.f;
    float      m_lastScale           = 1.f;
    float      m_zoomX               = 1.f;
};


/*
    setOptions(const RowOptions& opts):
    - Params:   const RowOptions& opts
    - Returns:  void
    - Desc:     Replaces the row's options and marks it dirty so the new
                appearance is picked up on the next draw.
*/
inline void Row::setOptions(const RowOptions& opts) {
    m_options = opts;
    m_dirty   = true;
}

}
