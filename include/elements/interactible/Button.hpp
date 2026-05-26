#pragma once

#include "../containers/Row.hpp"
#include "../decoration/Text.hpp"

namespace uilo {

class ButtonOptions {
public:
    ButtonOptions() = default;

    ButtonOptions& setColor(const Color& c) { m_color    = c; return *this; }
    ButtonOptions& setRounding(float r)          { m_rounding = r; return *this; }
    ButtonOptions& setLabel(Text* t)             { m_label    = t; return *this; }

    Color getColor()    const { return m_color; }
    float     getRounding() const { return m_rounding; }
    Text*     getLabel()    const { return m_label; }

private:
    Color m_color    = Color{0,0,0,0};
    float     m_rounding = 0.f;
    Text*     m_label    = nullptr;
};

class Button : public Row {
public:
    explicit Button(Modifier modifier, ButtonOptions options = {}, const std::string& name = "");

    const ButtonOptions& getOptions() const { return m_buttonOptions; }
    void setOptions(const ButtonOptions& opts);

    bool checkLeftClick(const Vec2f& mousePosition) override;
    bool checkRightClick(const Vec2f& mousePosition) override;
    bool checkHover(const Vec2f& mousePosition) override;
    bool checkScroll(const Vec2f& mousePosition, float delta) override;

private:
    ButtonOptions m_buttonOptions;
};

}