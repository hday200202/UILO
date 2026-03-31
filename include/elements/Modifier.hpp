#pragma once

#include <functional>

#include "../utils/Utils.hpp"
#include "../math/Vec2.hpp"
#include "../graphics/Color.hpp"

namespace uilo {

using FuncPtr = std::function<void()>;
using ScrollFuncPtr = std::function<void(float)>;

class Modifier {
public:
    Modifier() = default;

    Modifier& setWidth(Dimension dim);
    Modifier& setHeight(Dimension dim);
    Modifier& setColor(const Color& color);
    Modifier& setAlign(Align alignment);
    Modifier& setOnLeftClick(FuncPtr leftClick);
    Modifier& setOnRightClick(FuncPtr rightClick);
    Modifier& setOnHover(FuncPtr hover, float delay = 0.f);
    Modifier& setOnScroll(ScrollFuncPtr scroll);
    Modifier& setPadding(float padding);
    Modifier& setVisible(bool visible);
    Modifier& setFreePosition(const Vec2f& freePos);
    Modifier& setFitContentWidth(bool fit);
    Modifier& setFitContentHeight(bool fit);
    Modifier& setRounded(float radius);

    Dimension getWidth()                    const;
    Dimension getHeight()                   const;
    Color getColor()                        const;
    Align getAlign()                        const;
    const FuncPtr& getOnLeftClick()         const;
    const FuncPtr& getOnRightClick()        const;
    const FuncPtr& getOnHover()             const;
    const ScrollFuncPtr& getOnScroll()      const;
    float getHoverDelay()                   const;
    float getPadding()                      const;
    bool getVisible()                       const;
    Vec2f getFreePosition()                 const;
    bool getFitContentWidth()               const;
    bool getFitContentHeight()              const;
    float getCornerRadius()                 const;

private:
    Dimension m_width       = 100_pct;
    Dimension m_height      = 100_pct;
    Color m_color           = Colors::White;
    Align m_align           = Align::LEFT | Align::TOP;
    FuncPtr m_onLeftClick;
    FuncPtr m_onRightClick;
    FuncPtr m_onHover;
    ScrollFuncPtr m_onScroll;
    float m_hoverDelay      = 0.f;
    float m_padding         = 0.f;
    bool m_visible          = true;
    Vec2f m_freePosition    = {0, 0};
    bool m_fitContentWidth  = false;
    bool m_fitContentHeight = false;
    float m_cornerRadius    = 0.f;
};

}