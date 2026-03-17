#pragma once

#include <vector>
#include "Bounds.hpp"
#include "Modifier.hpp"
#include "../renderer/Renderer.hpp"
#include "../input/Input.hpp"

namespace uilo {

class Element {
public:
    Element() = default;
    virtual ~Element() = default;

    void setModifier(Modifier& modifier) { m_modifier = modifier; }

    Bounds getBounds() const { return m_bounds; }
    Modifier& getModifier() { return m_modifier; }

protected:
    Bounds m_bounds;
    Modifier m_modifier;

    virtual void update(const float dt) {}
    virtual void render(Renderer& renderer) {}
    virtual void checkHover(const Vec2f& mousePosition) {}
    virtual void checkClick(const Vec2f& mousePosition) {}
    virtual void checkScoll(const Vec2f& mousePosition, float vDelta) {}
};

}