#pragma once

#include "../Element.hpp"

namespace uilo {

class SpacerOptions {
public:
    SpacerOptions() = default;
};

class Spacer : public Element {
public:
    explicit Spacer(Modifier modifier, SpacerOptions options = {}, const std::string& name = "");

    void update(sf::FloatRect& parentBounds, float dt) override;
    void render(sf::RenderTarget& target) override;
};

}