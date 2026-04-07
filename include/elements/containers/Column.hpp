#pragma once

#include "Container.hpp"

namespace uilo {

class Column : public Container {
public:
    using Container::Container;

    void update(Bounds& parentBounds, float dt) override;
    void render(Renderer& renderer) override;
};

} // namespace uilo