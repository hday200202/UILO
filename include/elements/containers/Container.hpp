#pragma once

#include <vector>
#include <initializer_list>
#include "../Element.hpp"

namespace uilo {

using contains = std::initializer_list<Element*>;

class Container : public Element {
public:
    Container(Modifier modifier, contains children, const std::string& name);

    // Sub-classes must override
    void update(Bounds& parentBounds, float dt) override = 0;
    void render(Renderer& renderer) override = 0;

    bool checkRightClick(const Vec2f& mousePosition) override;
    bool checkLeftClick(const Vec2f& mousePosition) override;
    bool checkHover(const Vec2f& mousePosition) override;
    bool checkScroll(const Vec2f& mousePosition, float delta) override;
    
    void setUilo(UILO& uiloRef) override;

protected:
    std::vector<Element*> m_children;
};

}