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
Modifier& Modifier::setOuterPadding(float padding) { m_outerPadding = padding; return *this; }
Modifier& Modifier::setVisible(bool visible) { m_visible = visible; return *this; }
Modifier& Modifier::ignoreScroll(bool ignore) { m_ignoreScroll = ignore; return *this; }
Modifier& Modifier::setFreePosition(const Vec2f& freePos) { m_freePosition = freePos; return *this; }
Modifier& Modifier::setMaterial(const Material& m) { m_material = m; return *this; }
// setOnLeftClick / setOnRightClick / setOnHover / setOnScroll are templated
// in the header so they can auto-wrap user lambdas with different shapes.

Dimension Modifier::getWidth() const { return m_width; }
Dimension Modifier::getHeight() const { return m_height; }
Align Modifier::getAlign() const { return m_align; }
const FuncPtr& Modifier::getOnLeftClick() const { return m_onLeftClick; }
const FuncPtr& Modifier::getOnRightClick() const { return m_onRightClick; }
const FuncPtr& Modifier::getOnHoverEnter() const { return m_onHoverEnter; }
const FuncPtr& Modifier::getOnHoverExit() const { return m_onHoverExit; }
const FuncPtr& Modifier::getOnUpdateStart() const { return m_onUpdateStart; }
const FuncPtr& Modifier::getOnUpdateEnd() const { return m_onUpdateEnd; }
const ScrollFuncPtr& Modifier::getOnScroll() const { return m_onScroll; }
float Modifier::getOuterPadding() const { return m_outerPadding; }
bool Modifier::getVisible() const { return m_visible; }
bool Modifier::getIgnoreScroll() const { return m_ignoreScroll; }
Vec2f Modifier::getFreePosition() const { return m_freePosition; }
const Material& Modifier::getMaterial() const { return m_material; }

} // namespace uilo
