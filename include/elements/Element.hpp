#pragma once

#include "Modifier.hpp"
#include "../Common.hpp"

namespace uilo {

class Element {
public:
    Element() = default;
    virtual ~Element() = default;

protected:
    Rect m_bounds;
    Rect m_pastBounds;

    Modifier m_modifier;

    void update(float dt);
    void updateChildren();

    void checkClick();
    void checkScroll();
};

}