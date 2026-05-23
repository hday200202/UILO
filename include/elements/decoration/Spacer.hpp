#pragma once

#include "../Element.hpp"

namespace uilo {

class Spacer : public Element {
public:
    Spacer(Modifier modifier, const std::string& name = "");

    void update(sf::FloatRect& parentBounds, float dt) override;
    void render(sf::RenderTarget& target) override;
};

}