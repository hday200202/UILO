#pragma once

#include "../containers/Row.hpp"
#include "../decoration/Text.hpp"

namespace uilo {

class ButtonOptions {
public:
    ButtonOptions() = default;

    ButtonOptions& setColor(const sf::Color& c) { m_color    = c; return *this; }
    ButtonOptions& setRounding(float r)          { m_rounding = r; return *this; }
    ButtonOptions& setLabel(Text* t)             { m_label    = t; return *this; }

    sf::Color getColor()    const { return m_color; }
    float     getRounding() const { return m_rounding; }
    Text*     getLabel()    const { return m_label; }

private:
    sf::Color m_color    = sf::Color::Transparent;
    float     m_rounding = 0.f;
    Text*     m_label    = nullptr;
};

class Button : public Row {
public:
    explicit Button(Modifier modifier, ButtonOptions options = {}, const std::string& name = "");

    const ButtonOptions& getOptions() const { return m_buttonOptions; }
    void setOptions(const ButtonOptions& opts);

    bool checkLeftClick(const sf::Vector2f& mousePosition) override;
    bool checkRightClick(const sf::Vector2f& mousePosition) override;
    bool checkHover(const sf::Vector2f& mousePosition) override;
    bool checkScroll(const sf::Vector2f& mousePosition, float delta) override;

private:
    ButtonOptions m_buttonOptions;
};

}