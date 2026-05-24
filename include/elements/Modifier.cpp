#include "Modifier.hpp"
#include <algorithm>

namespace uilo {

Modifier& Modifier::setWidth(Dimension dim) {
    if (dim.percent) dim.value = std::clamp(dim.value, 1.f, 100.f);
    m_width = dim;
    return *this;
}

Modifier& Modifier::setHeight(Dimension dim) {
    if (dim.percent) dim.value = std::clamp(dim.value, 1.f, 100.f);
    m_height = dim;
    return *this;
}

Modifier& Modifier::setAlign(Align alignment) { m_align = alignment; return *this; }
Modifier& Modifier::setOnLeftClick(FuncPtr leftClick) { m_onLeftClick = leftClick; return *this; }
Modifier& Modifier::setOnRightClick(FuncPtr rightClick) { m_onRightClick = rightClick; return *this; }
Modifier& Modifier::setOnScroll(ScrollFuncPtr scroll) { m_onScroll = scroll; return *this; }
Modifier& Modifier::setOuterPadding(float padding) { m_outerPadding = padding; return *this; }
Modifier& Modifier::setVisible(bool visible) { m_visible = visible; return *this; }
Modifier& Modifier::setFreePosition(const sf::Vector2f& freePos) { m_freePosition = freePos; return *this; }
Modifier& Modifier::setOnHover(FuncPtr hover, float delay) {
    m_onHover = hover;
    m_hoverDelay = delay;
    return *this;
}

Dimension Modifier::getWidth() const { return m_width; }
Dimension Modifier::getHeight() const { return m_height; }
Align Modifier::getAlign() const { return m_align; }
const FuncPtr& Modifier::getOnLeftClick() const { return m_onLeftClick; }
const FuncPtr& Modifier::getOnRightClick() const { return m_onRightClick; }
const FuncPtr& Modifier::getOnHover() const { return m_onHover; }
const ScrollFuncPtr& Modifier::getOnScroll() const { return m_onScroll; }
float Modifier::getHoverDelay() const { return m_hoverDelay; }
float Modifier::getOuterPadding() const { return m_outerPadding; }
bool Modifier::getVisible() const { return m_visible; }
sf::Vector2f Modifier::getFreePosition() const { return m_freePosition; }

} // namespace uilo
