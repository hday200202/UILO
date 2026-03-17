#pragma once

#include <functional>
#include "Alignment.hpp"
#include "../utils/Color.hpp"
#include "../utils/Dimension.hpp"

using FuncPtr = std::function<void()>;

namespace uilo {

class Modifier {
public:
    Modifier() = default;

    Modifier& setWidth(Dimension width)         { m_width       = width;    return *this; }
    Modifier& setHeight(Dimension height)       { m_height      = height;   return *this; }
    Modifier& setColor(Color color)             { m_color       = color;    return *this; }
    Modifier& setRound(float radius)            { m_roundRadius = radius;   return *this; }
    Modifier& setOnLClick(FuncPtr lClick)       { m_lClick      = lClick;   return *this; }
    Modifier& setOnRClick(FuncPtr rClick)       { m_rClick      = rClick;   return *this; }
    Modifier& setOnHover(FuncPtr hover)         { m_hover       = hover;    return *this; }
    Modifier& setOnScrollUp(FuncPtr up)         { m_uScroll     = up;       return *this; }
    Modifier& setOnScrollDown(FuncPtr down)     { m_dScroll     = down;     return *this; }
    Modifier& align(Align align)                { m_alignment   = align;    return *this; }
    Modifier& setVisible(bool visible)          { m_visible     = visible;  return *this; }

    Dimension getWidth()                const   { return m_width;       }
    Dimension getHeight()               const   { return m_height;      }
    Color getColor()                    const   { return m_color;       }
    float getRoundRadius()              const   { return m_roundRadius; }
    const FuncPtr& getOnLClick()        const   { return m_lClick;      }
    const FuncPtr& getOnRClick()        const   { return m_rClick;      }
    const FuncPtr& getOnHover()         const   { return m_hover;       }
    const FuncPtr& getOnScrollUp()      const   { return m_uScroll;     }
    const FuncPtr& getOnScrollDown()    const   { return m_dScroll;     }
    Align getAlignment()                const   { return m_alignment;   }
    bool isVisible()                    const   { return m_visible;     }

protected:
    Dimension m_width       = {1.f, true};
    Dimension m_height      = {1.f, true};
    Color m_color           = {255, 255, 255, 255};
    float m_roundRadius     = 0.f;

    FuncPtr m_lClick        = nullptr;
    FuncPtr m_rClick        = nullptr;
    FuncPtr m_hover         = nullptr;
    FuncPtr m_uScroll       = nullptr;
    FuncPtr m_dScroll       = nullptr;

    Align m_alignment       = Align::LEFT | Align::TOP;

    bool m_visible          = true;
};

}