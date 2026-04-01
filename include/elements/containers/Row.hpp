#pragma once

#include "Container.hpp"

namespace uilo {

class Row : public Container {
public:
    using Container::Container;

    void update(Bounds& parentBounds, float dt) override;
    void render(Renderer& renderer) override;

protected:

};

}