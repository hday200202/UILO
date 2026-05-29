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

private:
    RowOptions m_options;
    float      m_scrollOffset = 0.f;
    float      m_contentWidth = 0.f;
    float      m_lastScale    = 1.f;
};

}