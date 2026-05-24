#pragma once

#include "Container.hpp"

namespace uilo {

class RowOptions {
public:
    RowOptions() = default;
};

class Row : public Container {
public:
    using Container::Container;

    explicit Row(Modifier modifier, RowOptions options, contains children, const std::string& name = "");

    void update(sf::FloatRect& parentBounds, float dt) override;
    void render(sf::RenderTarget& target) override;
};

}