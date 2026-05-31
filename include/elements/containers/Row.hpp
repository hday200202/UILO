#pragma once

#include "Container.hpp"

namespace uilo {

class RowOptions {
public:
    RowOptions() = default;

    RowOptions& setColor(const Color& c)    { m_color       = c;  return *this; }
    RowOptions& setColorRole(const std::string& r) { m_colorRole = r; return *this; }
    RowOptions& setRounding(float r)            { m_rounding    = r;  return *this; }
    RowOptions& setScrollable(bool v)           { m_scrollable  = v;  return *this; }
    RowOptions& setScrollSpeed(float s)         { m_scrollSpeed = s;  return *this; }
    RowOptions& setScrollMin(float v)           { m_scrollMin = v; m_scrollMinSet = true; return *this; }
    RowOptions& setScrollMax(float v)           { m_scrollMax = v; m_scrollMaxSet = true; return *this; }
    RowOptions& setScrollLink(const std::string& id) { m_scrollLink = id; return *this; }

    // Subdivision grid -------------------------------------------------
    // baseInterval: spacing between primary lines in unscaled content px.
    // Lines are drawn relative to scroll so they move with the content.
    // major/minor define a repeated hierarchy. For example, 1 major and 3
    // minor subdivisions yields four equal slices per division.
    RowOptions& setSubDivisions(float px)                     { m_subDivisions = px;        return *this; }
    RowOptions& setSubDivisionMajor(unsigned int count)       { m_subDivMajor = count;      return *this; }
    RowOptions& setSubDivisionMinor(unsigned int count)       { m_subDivMinor = count;      return *this; }
    RowOptions& setSubDivisionColor(Color c)                  { m_subDivColor = c;          return *this; }
    RowOptions& setSubDivisionColorRole(const std::string& r) { m_subDivColorRole = r;      return *this; }
    RowOptions& setSubDivisionMinScreenPx(float px)           { m_subDivMinPx = px;         return *this; }
    RowOptions& setSubDivisionResubdivideMinScreenPx(float px){ m_subDivResubdivideMinPx = px; return *this; }

    // Zoom -------------------------------------------------------------
    // When enabled, pinch/scroll-zoom scales content widths and the grid.
    RowOptions& setZoomableX(bool v)     { m_zoomableX = v;   return *this; }
    RowOptions& setZoomMin(float v)      { m_zoomMin   = v;   return *this; }
    RowOptions& setZoomMax(float v)      { m_zoomMax   = v;   return *this; }
    RowOptions& setZoomLink(const std::string& id) { m_zoomLink = id; return *this; }

    Color getColor()       const { return m_color; }
    const std::string& getColorRole() const { return m_colorRole; }
    float     getRounding()    const { return m_rounding; }
    bool      getScrollable()  const { return m_scrollable; }
    float     getScrollSpeed() const { return m_scrollSpeed; }
    float     getScrollMin()   const { return m_scrollMin; }
    float     getScrollMax()   const { return m_scrollMax; }
    bool      hasScrollMin()   const { return m_scrollMinSet; }
    bool      hasScrollMax()   const { return m_scrollMaxSet; }
    const std::string& getScrollLink() const { return m_scrollLink; }

    float              getSubDivisions()           const { return m_subDivisions; }
    unsigned int       getSubDivisionMajor()       const { return m_subDivMajor; }
    unsigned int       getSubDivisionMinor()       const { return m_subDivMinor; }
    Color              getSubDivisionColor()       const { return m_subDivColor; }
    const std::string& getSubDivisionColorRole()   const { return m_subDivColorRole; }
    float              getSubDivisionMinScreenPx() const { return m_subDivMinPx; }
    float              getSubDivisionResubdivideMinScreenPx() const { return m_subDivResubdivideMinPx; }
    bool               getZoomableX()              const { return m_zoomableX; }
    float              getZoomMin()                const { return m_zoomMin; }
    float              getZoomMax()                const { return m_zoomMax; }
    const std::string& getZoomLink() const { return m_zoomLink; }

private:
    Color m_color       = Color{0,0,0,0};
    std::string m_colorRole;
    float     m_rounding    = 0.f;
    bool      m_scrollable  = false;
    float     m_scrollSpeed = 40.f;
    float     m_scrollMin   = 0.f;
    float     m_scrollMax   = 0.f;
    bool      m_scrollMinSet = false;
    bool      m_scrollMaxSet = false;
    std::string m_scrollLink;

    float       m_subDivisions    = 0.f;
    unsigned int m_subDivMajor    = 1;
    unsigned int m_subDivMinor    = 3;
    Color       m_subDivColor     = Color{255, 255, 255, 30};
    std::string m_subDivColorRole;
    float       m_subDivMinPx     = 4.f;
    float       m_subDivResubdivideMinPx = 24.f;
    bool        m_zoomableX       = false;
    float       m_zoomMin         = 0.1f;
    float       m_zoomMax         = 50.f;
    std::string m_zoomLink;
};

class Row : public Container {
public:
    using Container::Container;

    explicit Row(Modifier modifier, RowOptions options, contains children, const std::string& name = "");

    const RowOptions& getOptions() const   { return m_options; }
    RowOptions&       getOptions()         { return m_options; }
    void setOptions(const RowOptions& opts) { m_options = opts; m_dirty = true; }

    void update(Rectf& parentBounds, float dt) override;
    void render() override;
    bool checkScroll(const Vec2f& mousePosition, float delta, bool precise = false, bool momentum = false) override;
    bool checkScroll(const Vec2f& mousePosition, Vec2f delta, bool precise = false, bool momentum = false) override;
    bool checkZoom(const Vec2f& mousePosition, float magnification) override;

    float getZoomX() const  { return m_zoomX; }
    void  setZoomX(float z);

private:
    RowOptions m_options;
    float      m_scrollOffset = 0.f;
    float      m_contentWidth = 0.f;
    float      m_scrollViewportWidth = 0.f;
    float      m_scrollViewportX     = 0.f;
    float      m_lastScale    = 1.f;
    float      m_zoomX        = 1.f;
};

}