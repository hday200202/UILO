#include "Column.hpp"

namespace uilo {

void Column::update(Bounds& parentBounds, float dt) {
    pruneChildren();
    resize(parentBounds);

    // Group visible children by vertical alignment and sum heights
    std::vector<Element*> top, mid, bot;
    float totalFixed = 0.f, totalPct = 0.f;

    for (auto* child : m_children) {
        if (!child->getModifier().getVisible()) continue;

        Align align = child->getModifier().getAlign();
        if      (hasAlign(align, Align::BOTTOM))   bot.push_back(child);
        else if (hasAlign(align, Align::CENTER_Y)) mid.push_back(child);
        else                                       top.push_back(child);

        Dimension dim = child->getModifier().getHeight();
        if (dim.percent) totalPct   += dim.value;
        else             totalFixed += dim.value;
    }

    if (top.empty() && mid.empty() && bot.empty()) return;

    // Pct children share this slot height so each resolves to its fair share of remaining space
    const float remaining = m_bounds.size.y - totalFixed;
    const float pctSlotH  = (totalPct > 0.f) ? (remaining * 100.f / totalPct) : 0.f;

    // Actual pixel height a child occupies after resize
    auto resolvedH = [&](Element* e) -> float {
        Dimension dim = e->getModifier().getHeight();
        return dim.percent ? (dim.value / 100.f * pctSlotH) : dim.value;
    };

    // Slot size.y to feed into resize so it produces the correct resolved height
    auto slotSizeY = [&](Element* e) -> float {
        Dimension dim = e->getModifier().getHeight();
        return dim.percent ? pctSlotH : dim.value;
    };

    // Offset slot.position.y so the child always lands at cursorY regardless of its vertical align flag
    auto slotPosY = [&](Element* e, float cursorY) -> float {
        float sh = slotSizeY(e), rh = resolvedH(e);
        Align align = e->getModifier().getAlign();
        if      (hasAlign(align, Align::BOTTOM))   return cursorY - (sh - rh);
        else if (hasAlign(align, Align::CENTER_Y)) return cursorY - (sh - rh) * 0.5f;
        return cursorY;
    };

    // Pre-compute group block heights to position mid and bot groups
    float midH = 0.f, botH = 0.f;
    for (auto* e : mid) midH += resolvedH(e);
    for (auto* e : bot) botH += resolvedH(e);

    float yPos = m_bounds.top();
    for (auto* e : top) {
        Bounds slot;
        slot.position = { m_bounds.position.x, slotPosY(e, yPos) };
        slot.size     = { m_bounds.size.x, slotSizeY(e) };
        e->update(slot, dt);
        yPos += resolvedH(e);
    }

    yPos = m_bounds.top() + (m_bounds.size.y - midH) * 0.5f;
    for (auto* e : mid) {
        Bounds slot;
        slot.position = { m_bounds.position.x, slotPosY(e, yPos) };
        slot.size     = { m_bounds.size.x, slotSizeY(e) };
        e->update(slot, dt);
        yPos += resolvedH(e);
    }

    yPos = m_bounds.bottom() - botH;
    for (auto* e : bot) {
        Bounds slot;
        slot.position = { m_bounds.position.x, slotPosY(e, yPos) };
        slot.size     = { m_bounds.size.x, slotSizeY(e) };
        e->update(slot, dt);
        yPos += resolvedH(e);
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