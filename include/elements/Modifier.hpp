#pragma once

#include <functional>
#include "../../include/utils/Math.hpp"
#include "../utils/Utils.hpp"

namespace uilo {

using FuncPtr = std::function<void()>;
using ScrollFuncPtr = std::function<void(float)>;

class Modifier {
public:
    Modifier() = default;

    Modifier& setWidth(Dimension dim);
    Modifier& setHeight(Dimension dim);
    Modifier& setAlign(Align alignment);
    Modifier& setOnLeftClick(FuncPtr leftClick);
    Modifier& setOnRightClick(FuncPtr rightClick);
    Modifier& setOnHover(FuncPtr hover, float delay = 0.f);
    Modifier& setOnScroll(ScrollFuncPtr scroll);
    Modifier& setOuterPadding(float padding);
    Modifier& setVisible(bool visible);
    Modifier& setFreePosition(const Vec2f& freePos);

    Dimension getWidth()                const;
    Dimension getHeight()               const;
    Align getAlign()                    const;
    const FuncPtr& getOnLeftClick()     const;
    const FuncPtr& getOnRightClick()    const;
    const FuncPtr& getOnHover()         const;
    const ScrollFuncPtr& getOnScroll()  const;
    float getHoverDelay()               const;
    float getOuterPadding()             const;
    bool getVisible()                   const;
    Vec2f getFreePosition() const;

private:
    Dimension m_width                   = 100_pct;
    Dimension m_height                  = 100_pct;
    Align m_align                       = Align::Left | Align::Top;
    FuncPtr m_onLeftClick;
    FuncPtr m_onRightClick;
    FuncPtr m_onHover;
    ScrollFuncPtr m_onScroll;
    float m_hoverDelay                  = 0.f;
    float m_outerPadding                = 0.f;
    bool m_visible                      = true;
    Vec2f m_freePosition = {0.f, 0.f};
};

}