#pragma once

#include "Container.hpp"

namespace uilo {

class ColumnOptions {
public:
    ColumnOptions() = default;

    ColumnOptions& setColor(const sf::Color& c) { m_color    = c; return *this; }
    ColumnOptions& setRounding(float r)          { m_rounding = r; return *this; }

    sf::Color getColor()    const { return m_color; }
    float     getRounding() const { return m_rounding; }

private:
    sf::Color m_color    = sf::Color::Transparent;
    float     m_rounding = 0.f;
};

class Column : public Container {
public:
    using Container::Container;

    explicit Column(Modifier modifier, ColumnOptions options, contains children, const std::string& name = "");

    const ColumnOptions& getOptions() const        { return m_options; }
    void setOptions(const ColumnOptions& opts)      { m_options = opts; }

    void update(sf::FloatRect& parentBounds, float dt) override;
    void render(sf::RenderTarget& target) override;

private:
    ColumnOptions m_options;
};

}