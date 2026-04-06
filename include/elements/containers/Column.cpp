#include "Column.hpp"

namespace uilo {

void Column::update(Bounds& parentBounds, float dt) {
    pruneChildren();
    resize(parentBounds);

    std::vector<Element*> top, mid, bot;
    float totalFixed = 0.f;

    for (auto* child : m_children) {
        if (!child->getModifier().getVisible()) continue;
        Align align = child->getModifier().getAlign();
        if      (hasAlign(align, Align::BOTTOM))   bot.push_back(child);
        else if (hasAlign(align, Align::CENTER_Y)) mid.push_back(child);
        else                                       top.push_back(child);
        Dimension dim = child->getModifier().getHeight();
        if (!dim.percent) totalFixed += dim.value;
    }

    const float remaining = m_bounds.size.y - totalFixed;
    const float pctSlotH  = remaining;

    auto resolvedH = [&](Element* e) -> float {
        Dimension dim = e->getModifier().getHeight();
        return dim.percent ? (dim.value / 100.f * pctSlotH) : dim.value;
    };
    auto slotSizeY = [&](Element* e) -> float {
        Dimension dim = e->getModifier().getHeight();
        return dim.percent ? pctSlotH : dim.value;
    };

    float midH = 0.f, botH = 0.f;
    for (auto* e : mid) midH += resolvedH(e);
    for (auto* e : bot) botH += resolvedH(e);

    auto layoutGroup = [&](std::vector<Element*>& group, float startY) {
        float cursorY = startY;
        for (auto* child : group) {
            float sh = slotSizeY(child);
            float rh = resolvedH(child);
            Align align = child->getModifier().getAlign();
            // Adjust slot start so that after resize() applies vertical alignment
            // the element lands exactly at cursorY (avoids double-centering pct slots).
            float slotY;
            if      (hasAlign(align, Align::BOTTOM))   slotY = cursorY + rh - sh;
            else if (hasAlign(align, Align::CENTER_Y)) slotY = cursorY - (sh - rh) * 0.5f;
            else                                       slotY = cursorY;
            Bounds slot;
            slot.position = { m_bounds.position.x, slotY };
            slot.size     = { m_bounds.size.x, sh };
            child->update(slot, dt);
            cursorY += rh;
        }
    };

    layoutGroup(top, m_bounds.top());
    layoutGroup(mid, m_bounds.top() + (m_bounds.size.y - midH) * 0.5f);
    layoutGroup(bot, m_bounds.bottom() - botH);
}

void Column::render(Renderer& renderer) {
    Color c = m_modifier.getColor();
    float radius = m_modifier.getCornerRadius();
    if (c.a > 0) {
        Rect rect;
        rect.setPosition(m_bounds.position);
        rect.setSize(m_bounds.size);
        rect.setColor(c);
        if (radius > 0.f)
            renderer.drawRoundedRect(rect, radius);
        else
            renderer.drawFilledRect(rect);
    }
    if (radius > 0.f)
        renderer.pushClipRoundedRect(m_bounds, radius);
    else
        renderer.pushClipRect(m_bounds);
    for (auto* child : m_children)
        child->render(renderer);
    renderer.popClip();
}

}