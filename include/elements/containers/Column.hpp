#pragma once

#include <optional>

#include "Container.hpp"
#include "../../utils/Theme.hpp"

namespace uilo {

/*
    ColumnOptions:
    - Desc:     Everything a Column draws that is not layout: background fill,
                gradient, corner rounding, border, scrolling, the subdivision
                grid and zoom. Colors come as a literal plus a role, where the
                role wins when it resolves against the active Palette and the
                literal is the fallback. A gradient takes precedence over a flat
                fill when active, and a Material on the Modifier takes
                precedence over both. Sizes are unscaled content pixels; UILO
                multiplies by its scale at layout time.
*/
class ColumnOptions {
public:
    ColumnOptions() = default;

    // Space kept outside this element, inside the slot its parent gave it. It
    // shrinks the element rather than displacing a sibling. Unset follows
    // Theme::setOuterPadding().
    ColumnOptions& setOuterPadding(float px)   { m_outerPadding = px; return *this; }
    ColumnOptions& clearOuterPadding()         { m_outerPadding.reset(); return *this; }
    float  getOuterPadding()     const { return Theme::resolveOuterPadding(m_outerPadding, 0.f); }

    // Space kept between this element's edge and the area its children are laid out in. Unset follows
    // Theme::setInnerPadding().
    ColumnOptions& setInnerPadding(float px)   { m_innerPadding = px; return *this; }
    ColumnOptions& clearInnerPadding()         { m_innerPadding.reset(); return *this; }
    float  getInnerPadding()     const { return Theme::resolveInnerPadding(m_innerPadding, 0.f); }

    // Background
    ColumnOptions& setColor(const Color& c)              { m_color = c; return *this; }
    ColumnOptions& setColorRole(const std::string& r)    { m_colorRole = r; return *this; }
    ColumnOptions& setGradient(const Gradient& g)        { m_gradient = g; return *this; }
    ColumnOptions& setGradientRole(const std::string& r) { m_gradientRole = r; return *this; }
    ColumnOptions& setRounding(float r)                  { m_rounding = r; return *this; }
    ColumnOptions& inheritRounding(const std::optional<float>& own, float fallback);

    // Outline: a border drawn inside the bounds, so enabling one never changes
    // the space the element takes.
    ColumnOptions& setOutlineColor(const Color& c)           { m_outlineColor = c; return *this; }
    ColumnOptions& setOutlineColorRole(const std::string& r) { m_outlineColorRole = r; return *this; }
    ColumnOptions& setOutlineThickness(float px)             { m_outlineThickness = px; return *this; }

    // Scrolling
    ColumnOptions& setScrollable(bool v)                { m_scrollable = v; return *this; }
    ColumnOptions& setScrollSpeed(float s)              { m_scrollSpeed = s; return *this; }
    ColumnOptions& setScrollMin(float v)                { m_scrollMin = v; m_scrollMinSet = true; return *this; }
    ColumnOptions& setScrollMax(float v)                { m_scrollMax = v; m_scrollMaxSet = true; return *this; }
    ColumnOptions& setScrollLink(const std::string& id) { m_scrollLink = id; return *this; }

    // Subdivision grid: setSubDivisions is the spacing between primary lines,
    // and major/minor define a repeated hierarchy within each division -- 1
    // major and 3 minor gives four equal slices. Lines are positioned relative
    // to the scroll offset, so they travel with the content.
    ColumnOptions& setSubDivisions(float px)                     { m_subDivisions = px; return *this; }
    ColumnOptions& setSubDivisionMajor(unsigned int count)       { m_subDivMajor = count; return *this; }
    ColumnOptions& setSubDivisionMinor(unsigned int count)       { m_subDivMinor = count; return *this; }
    ColumnOptions& setSubDivisionColor(Color c)                  { m_subDivColor = c; return *this; }
    ColumnOptions& setSubDivisionColorRole(const std::string& r) { m_subDivColorRole = r; return *this; }
    ColumnOptions& setSubDivisionMinScreenPx(float px)           { m_subDivMinPx = px; return *this; }
    ColumnOptions& setSubDivisionResubdivideMinScreenPx(float px) { m_subDivResubdivideMinPx = px; return *this; }
    ColumnOptions& setSubDivisionsMinDistance(float px)          { m_subDivMinDistance = px; return *this; }
    ColumnOptions& setSubDivisionsMaxDistance(float px)          { m_subDivMaxDistance = px; return *this; }
    ColumnOptions& setSubDivisionStripeEvery(unsigned int count) { m_subDivStripeEvery = count; return *this; }
    ColumnOptions& setSubDivisionStripeColor(Color c)            { m_subDivStripeColor = c; return *this; }
    ColumnOptions& setSubDivisionStripeColorRole(const std::string& r) { m_subDivStripeColorRole = r; return *this; }

    // Zoom: scales content heights and the grid under pinch or scroll-zoom.
    ColumnOptions& setZoomableY(bool v)               { m_zoomableY = v; return *this; }
    ColumnOptions& setZoomMin(float v)                { m_zoomMin = v; return *this; }
    ColumnOptions& setZoomMax(float v)                { m_zoomMax = v; return *this; }
    ColumnOptions& setZoomLink(const std::string& id) { m_zoomLink = id; return *this; }

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

    bool               getZoomableY() const { return m_zoomableY; }
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

    bool        m_zoomableY = false;
    float       m_zoomMin   = 0.1f;
    float       m_zoomMax   = 50.f;
    std::string m_zoomLink;
};


/*
    inheritRounding(const std::optional<float>& own, float fallback):
    - Params:   const std::optional<float>& own, float fallback
    - Returns:  ColumnOptions&
    - Desc:     Takes a rounding straight from a composite widget. `own` is
                whatever the widget was told, empty when nothing, and `fallback`
                is that widget's own default for when the theme is silent too.
                Passing it through unresolved, rather than as a number the
                widget already worked out, is what lets the theme keep reaching
                this element after it has been built.
*/
inline ColumnOptions& ColumnOptions::inheritRounding(
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
                inheritRounding (0 for a plain Column). Resolved on every read
                rather than cached, so changing the Theme restyles an element
                already on screen.
*/
inline float ColumnOptions::getRounding() const {
    return Theme::resolveRounding(m_rounding, m_roundingFallback);
}


/*
    Column:
    - Desc:     A Container that lays its children out top to bottom,
                distributing the vertical axis between them and giving each the
                full width. Optionally scrolls, zooms, and draws a subdivision
                grid behind its children. Row is the same class with the axes
                exchanged.
*/
class Column : public Container {
public:
    float getOuterPadding() const override { return m_options.getOuterPadding(); }
    float getInnerPadding() const override { return m_options.getInnerPadding(); }

    using Container::Container;

    explicit Column(
        Modifier modifier,
        ColumnOptions options,
        contains children,
        const std::string& name = ""
    );

    const ColumnOptions& getOptions() const { return m_options; }
    ColumnOptions&       getOptions()       { return m_options; }
    void                 setOptions(const ColumnOptions& opts);
    void                 setScrollOffset(float offset);

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

    float getZoomY() const { return m_zoomY; }
    void  setZoomY(float z);

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

    ColumnOptions m_options;
    float         m_scrollOffset         = 0.f;
    float         m_contentHeight        = 0.f;
    float         m_scrollViewportHeight = 0.f;
    float         m_scrollViewportY      = 0.f;
    float         m_lastScale            = 1.f;
    float         m_zoomY                = 1.f;
};


/*
    setOptions(const ColumnOptions& opts):
    - Params:   const ColumnOptions& opts
    - Returns:  void
    - Desc:     Replaces the column's options and marks it dirty so the new
                appearance is picked up on the next draw.
*/
inline void Column::setOptions(const ColumnOptions& opts) {
    m_options = opts;
    m_dirty   = true;
}

}
