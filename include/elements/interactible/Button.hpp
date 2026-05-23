#pragma once

#include "../containers/Row.hpp"
#include "../decoration/Text.hpp"

namespace uilo {

class Button : public Row {
public:
    Button(Modifier modifier, Text* text, const std::string& name = "");

    bool checkLeftClick(const sf::Vector2f& mousePosition) override;
    bool checkRightClick(const sf::Vector2f& mousePosition) override;
    bool checkHover(const sf::Vector2f& mousePosition) override;
    bool checkScroll(const sf::Vector2f& mousePosition, float delta) override;
};

}