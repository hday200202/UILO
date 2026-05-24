#pragma once

#include "../Element.hpp"

namespace uilo {

class SpacerOptions {
public:
    SpacerOptions() = default;

    SpacerOptions& setColor(const sf::Color& c) { m_color    = c; return *this; }
    SpacerOptions& setRounding(float r)          { m_rounding = r; return *this; }

    sf::Color getColor()    const { return m_color; }
    float     getRounding() const { return m_rounding; }

private:
    sf::Color m_color    = sf::Color::Transparent;
    float     m_rounding = 0.f;
};

class Spacer : public Element {
public:
    explicit Spacer(Modifier modifier, SpacerOptions options = {}, const std::string& name = "");

    const SpacerOptions& getOptions() const        { return m_options; }
    void setOptions(const SpacerOptions& opts)      { m_options = opts; }

    void update(sf::FloatRect& parentBounds, float dt) override;
    void render(sf::RenderTarget& target) override;

private:
    SpacerOptions m_options;
};

}