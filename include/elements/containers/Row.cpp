#include "Row.hpp"

namespace uilo {

void Row::update(Bounds& parentBounds, float dt) {
    pruneChildren();
    resize(parentBounds);

    float totalFixed = 0.f, totalPct = 0.f;
    for (auto* child : m_children) {
        if (!child->getModifier().getVisible()) continue;
        Dimension dim = child->getModifier().getWidth();
        if (dim.percent) totalPct   += dim.value;
        else             totalFixed += dim.value;
    }

    const float remaining = m_bounds.size.x - totalFixed;
    const float pctSlotW  = (totalPct > 0.f) ? (remaining * 100.f / totalPct) : 0.f;

    auto resolvedW = [&](Element* e) -> float {
        Dimension dim = e->getModifier().getWidth();
        return dim.percent ? (dim.value / 100.f * pctSlotW) : dim.value;
    };

    auto slotSizeX = [&](Element* e) -> float {
        Dimension dim = e->getModifier().getWidth();
        return dim.percent ? pctSlotW : dim.value;
    };

    float cursorX = m_bounds.left();
    for (auto* child : m_children) {
        if (!child->getModifier().getVisible()) continue;
        Bounds slot;
        slot.position = { cursorX, m_bounds.position.y };
        slot.size     = { slotSizeX(child), m_bounds.size.y };
        child->update(slot, dt);
        cursorX += resolvedW(child);
    }
}

void Row::render(Renderer& renderer) {
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
