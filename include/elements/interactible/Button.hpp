#pragma once

#include "../containers/Row.hpp"
#include "../decoration/Text.hpp"

namespace uilo {

class ButtonOptions {
public:
    ButtonOptions() = default;

    ButtonOptions& setLabel(Text* t) { m_label = t; return *this; }

    Text* getLabel() const { return m_label; }

private:
    Text* m_label = nullptr;
};

class Button : public Row {
public:
    explicit Button(Modifier modifier, ButtonOptions options = {}, const std::string& name = "");

    bool checkLeftClick(const sf::Vector2f& mousePosition) override;
    bool checkRightClick(const sf::Vector2f& mousePosition) override;
    bool checkHover(const sf::Vector2f& mousePosition) override;
    bool checkScroll(const sf::Vector2f& mousePosition, float delta) override;
};

}