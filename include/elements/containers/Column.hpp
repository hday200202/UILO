#pragma once

#include "Container.hpp"

namespace uilo {

class ColumnOptions {
public:
    ColumnOptions() = default;
};

class Column : public Container {
public:
    using Container::Container;

    explicit Column(Modifier modifier, ColumnOptions options, contains children, const std::string& name = "");

    void update(sf::FloatRect& parentBounds, float dt) override;
    void render(sf::RenderTarget& target) override;
};

}