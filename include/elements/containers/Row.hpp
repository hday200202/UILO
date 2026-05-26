#pragma once

#include "Container.hpp"

namespace uilo {

class RowOptions {
public:
    RowOptions() = default;

    RowOptions& setColor(const sf::Color& c)    { m_color       = c;  return *this; }
    RowOptions& setRounding(float r)            { m_rounding    = r;  return *this; }
    RowOptions& setScrollable(bool v)           { m_scrollable  = v;  return *this; }
    RowOptions& setScrollSpeed(float s)         { m_scrollSpeed = s;  return *this; }

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

class Row : public Container {
public:
    using Container::Container;

    explicit Row(Modifier modifier, RowOptions options, contains children, const std::string& name = "");

    const RowOptions& getOptions() const   { return m_options; }
    void setOptions(const RowOptions& opts) { m_options = opts; m_dirty = true; }

    void update(sf::FloatRect& parentBounds, float dt) override;
    void render(sf::RenderTarget& target) override;
    bool checkScroll(const sf::Vector2f& mousePosition, float delta) override;

private:
    RowOptions m_options;
    float      m_scrollOffset = 0.f;
    float      m_contentWidth = 0.f;
    float      m_lastScale    = 1.f;
};

}