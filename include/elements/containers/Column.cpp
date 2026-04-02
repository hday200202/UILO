#include "Column.hpp"

namespace uilo {

void Column::update(Bounds& parentBounds, float dt) {
    pruneChildren();
    resize(parentBounds);

    float totalFixed = 0.f, totalPct = 0.f;
    for (auto* child : m_children) {
        if (!child->getModifier().getVisible()) continue;
        Dimension dim = child->getModifier().getHeight();
        if (dim.percent) totalPct   += dim.value;
        else             totalFixed += dim.value;
    }

    const float remaining = m_bounds.size.y - totalFixed;
    const float pctSlotH  = (totalPct > 0.f) ? (remaining * 100.f / totalPct) : 0.f;

    auto resolvedH = [&](Element* e) -> float {
        Dimension dim = e->getModifier().getHeight();
        return dim.percent ? (dim.value / 100.f * pctSlotH) : dim.value;
    };

    auto slotSizeY = [&](Element* e) -> float {
        Dimension dim = e->getModifier().getHeight();
        return dim.percent ? pctSlotH : dim.value;
    };

    float cursorY = m_bounds.top();
    for (auto* child : m_children) {
        if (!child->getModifier().getVisible()) continue;
        Bounds slot;
        slot.position = { m_bounds.position.x, cursorY };
        slot.size     = { m_bounds.size.x, slotSizeY(child) };
        child->update(slot, dt);
        cursorY += resolvedH(child);
    }
}

void Column::render(Renderer& renderer) {
    Color c = m_modifier.getColor();
    if (c.a > 0) {
        Rect rect;
        rect.setPosition(m_bounds.position);
        rect.setSize(m_bounds.size);
        rect.setColor(c);
        float radius = m_modifier.getCornerRadius();
        if (radius > 0.f)
            renderer.drawRoundedRect(rect, radius);
        else
            renderer.drawFilledRect(rect);
    }
    for (auto* child : m_children)
        child->render(renderer);
}

}