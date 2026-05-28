#pragma once

#include "Container.hpp"

namespace uilo {

class ColumnOptions {
public:
    ColumnOptions() = default;

    ColumnOptions& setColor(const Color& c)    { m_color       = c;  return *this; }
    ColumnOptions& setColorRole(const std::string& r) { m_colorRole = r; return *this; }
    ColumnOptions& setRounding(float r)         { m_rounding    = r;  return *this; }
    ColumnOptions& setScrollable(bool v)        { m_scrollable  = v;  return *this; }
    ColumnOptions& setScrollSpeed(float s)      { m_scrollSpeed = s; return *this; }

    Color getColor()       const { return m_color; }
    const std::string& getColorRole() const { return m_colorRole; }
    float     getRounding()    const { return m_rounding; }
    bool      getScrollable()  const { return m_scrollable; }
    float     getScrollSpeed() const { return m_scrollSpeed; }

private:
    Color m_color       = Color{0,0,0,0};
    std::string m_colorRole;
    float     m_rounding    = 0.f;
    bool      m_scrollable  = false;
    float     m_scrollSpeed = 40.f;
};

class Column : public Container {
public:
    using Container::Container;

    explicit Column(Modifier modifier, ColumnOptions options, contains children, const std::string& name = "");

    const ColumnOptions& getOptions() const        { return m_options; }
    ColumnOptions&       getOptions()              { return m_options; }
    void setOptions(const ColumnOptions& opts)      { m_options = opts; m_dirty = true; }
    void setScrollOffset(float offset);

    void update(Rectf& parentBounds, float dt) override;
    void render() override;
    bool checkScroll(const Vec2f& mousePosition, float delta) override;

private:
    ColumnOptions m_options;
    float         m_scrollOffset  = 0.f;
    float         m_contentHeight = 0.f;
    float         m_lastScale     = 1.f;
};

}