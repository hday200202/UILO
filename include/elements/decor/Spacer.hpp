#pragma once

#include "../Element.hpp"

namespace uilo {

class Spacer : public Element {
public:
    Spacer(Modifier modifier, const std::string& name = "");

    void update(Bounds& parentBounds, float dt) override;
    void render(Renderer& renderer) override;
    bool checkRightClick(const Vec2f& mousePosition) override;
    bool checkLeftClick(const Vec2f& mousePosition) override;
    bool checkHover(const Vec2f& mousePosition) override;
    bool checkScroll(const Vec2f& mousePosition, float delta) override;
};

}