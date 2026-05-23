#pragma once

#include <SFML/Graphics.hpp>
#include <functional>
#include "../utils/Utils.hpp"

namespace uilo {

using FuncPtr = std::function<void()>;
using ScrollFuncPtr = std::function<void(float)>;

class Modifier {
public:
    Modifier() = default;

    Modifier& setWidth(Dimension dim);
    Modifier& setHeight(Dimension dim);
    Modifier& setColor(const sf::Color& color);
    Modifier& setAlign(Align alignment);
    Modifier& setOnLeftClick(FuncPtr leftClick);
    Modifier& setOnRightClick(FuncPtr rightClick);
    Modifier& setOnHover(FuncPtr hover, float delay = 0.f);
    Modifier& setOnScroll(ScrollFuncPtr scroll);
    Modifier& setOuterPadding(float padding);
    Modifier& setVisible(bool visible);
    Modifier& setFreePosition(const sf::Vector2f& freePos);
    Modifier& setRounding(float r);

    Dimension getWidth()                const;
    Dimension getHeight()               const;
    sf::Color getColor()                const;
    Align getAlign()                    const;
    const FuncPtr& getOnLeftClick()     const;
    const FuncPtr& getOnRightClick()    const;
    const FuncPtr& getOnHover()         const;
    const ScrollFuncPtr& getOnScroll()  const;
    float getHoverDelay()               const;
    float getOuterPadding()             const;
    bool getVisible()                   const;
    sf::Vector2f getFreePosition()      const;
    float getRounding()                 const;

private:
    Dimension m_width                   = 100_pct;
    Dimension m_height                  = 100_pct;
    sf::Color m_color                   = sf::Color::Transparent;
    Align m_align                       = Align::Left | Align::Top;
    FuncPtr m_onLeftClick;
    FuncPtr m_onRightClick;
    FuncPtr m_onHover;
    ScrollFuncPtr m_onScroll;
    float m_hoverDelay                  = 0.f;
    float m_outerPadding                = 0.f;
    bool m_visible                      = true;
    sf::Vector2f m_freePosition         = {0.f, 0.f};
    float m_rounding                    = 0.f;
};

}