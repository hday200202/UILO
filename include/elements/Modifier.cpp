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

Modifier& Modifier::setColor(const Color& color) { m_color = color; return *this; }
Modifier& Modifier::setAlign(Align alignment) { m_align = alignment; return *this; }
Modifier& Modifier::setOnLeftClick(FuncPtr leftClick) { m_onLeftClick = leftClick; return *this; }
Modifier& Modifier::setOnRightClick(FuncPtr rightClick) { m_onRightClick = rightClick; return *this; }
Modifier& Modifier::setOnScroll(ScrollFuncPtr scroll) { m_onScroll = scroll; return *this; }
Modifier& Modifier::setPadding(float padding) { m_padding = padding; return *this; }
Modifier& Modifier::setVisible(bool visible) { m_visible = visible; return *this; }
Modifier& Modifier::setFreePosition(const Vec2f& freePos) { m_freePosition = freePos; return *this; }
Modifier& Modifier::setFitContentWidth(bool fit) { m_fitContentWidth = fit; return *this; }
Modifier& Modifier::setFitContentHeight(bool fit) { m_fitContentHeight = fit; return *this; }
Modifier& Modifier::setRounded(float radius) { m_cornerRadius = radius; return *this; }

Modifier& Modifier::setOnHover(FuncPtr hover, float delay) {
    m_onHover = hover;
    m_hoverDelay = delay;
    return *this;
}

Dimension Modifier::getWidth() const { return m_width; }
Dimension Modifier::getHeight() const { return m_height; }
Color Modifier::getColor() const { return m_color; }
Align Modifier::getAlign() const { return m_align; }
const FuncPtr& Modifier::getOnLeftClick() const { return m_onLeftClick; }
const FuncPtr& Modifier::getOnRightClick() const { return m_onRightClick; }
const FuncPtr& Modifier::getOnHover() const { return m_onHover; }
const ScrollFuncPtr& Modifier::getOnScroll() const { return m_onScroll; }
float Modifier::getHoverDelay() const { return m_hoverDelay; }
float Modifier::getPadding() const { return m_padding; }
bool Modifier::getVisible() const { return m_visible; }
Vec2f Modifier::getFreePosition() const { return m_freePosition; }
bool Modifier::getFitContentWidth() const { return m_fitContentWidth; }
bool Modifier::getFitContentHeight() const { return m_fitContentHeight; }
float Modifier::getCornerRadius() const { return m_cornerRadius; }

} // namespace uilo
