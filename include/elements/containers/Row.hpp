#pragma once

#include "Container.hpp"

namespace uilo {

class RowOptions {
public:
    RowOptions() = default;

    RowOptions& setColor(const sf::Color& c) { m_color    = c; return *this; }
    RowOptions& setRounding(float r)          { m_rounding = r; return *this; }

    sf::Color getColor()    const { return m_color; }
    float     getRounding() const { return m_rounding; }

private:
    sf::Color m_color    = sf::Color::Transparent;
    float     m_rounding = 0.f;
};

class Row : public Container {
public:
    using Container::Container;

    explicit Row(Modifier modifier, RowOptions options, contains children, const std::string& name = "");

    const RowOptions& getOptions() const   { return m_options; }
    void setOptions(const RowOptions& opts) { m_options = opts; }

    void update(sf::FloatRect& parentBounds, float dt) override;
    void render(sf::RenderTarget& target) override;

private:
    RowOptions m_options;
};

}