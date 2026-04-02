#pragma once

#include "../Element.hpp"

namespace uilo {

class Spacer : public Element {
public:
    Spacer(Modifier modifier, const std::string& name = "");

    void update(Bounds& parentBounds, float dt) override;
    void render(Renderer& renderer) override;
};

}