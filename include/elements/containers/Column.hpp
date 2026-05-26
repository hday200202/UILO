#pragma once

#include "Container.hpp"

namespace uilo {

class ColumnOptions {
public:
    ColumnOptions() = default;

    ColumnOptions& setColor(const sf::Color& c) { m_color       = c;  return *this; }
    ColumnOptions& setRounding(float r)         { m_rounding    = r;  return *this; }
    ColumnOptions& setScrollable(bool v)        { m_scrollable  = v;  return *this; }
    ColumnOptions& setScrollSpeed(float s)      { m_scrollSpeed = s; return *this; }

    sf::Color getColor()       const { return m_color; }
    float     getRounding()    const { return m_rounding; }
    bool      getScrollable()  const { return m_scrollable; }
    float     getScrollSpeed() const { return m_scrollSpeed; }

private:
    sf::Color m_color       = sf::Color::Transparent;
    float     m_rounding    = 0.f;
    bool      m_scrollable  = false;
    float     m_scrollSpeed = 40.f;
};

class Column : public Container {
public:
    using Container::Container;

    explicit Column(Modifier modifier, ColumnOptions options, contains children, const std::string& name = "");

    const ColumnOptions& getOptions() const        { return m_options; }
    void setOptions(const ColumnOptions& opts)      { m_options = opts; m_dirty = true; }
    void setScrollOffset(float offset);

    void update(sf::FloatRect& parentBounds, float dt) override;
    void render(sf::RenderTarget& target) override;
    bool checkScroll(const sf::Vector2f& mousePosition, float delta) override;

private:
    ColumnOptions m_options;
    float         m_scrollOffset  = 0.f;
    float         m_contentHeight = 0.f;
    float         m_lastScale     = 1.f;
};

}