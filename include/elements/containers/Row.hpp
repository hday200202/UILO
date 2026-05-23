#pragma once

#include "Container.hpp"

namespace uilo {

class Row : public Container {
public:
    using Container::Container;

    void update(sf::FloatRect& parentBounds, float dt) override;
    void render(sf::RenderTarget& target) override;
};

}