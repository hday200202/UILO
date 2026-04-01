#include "Row.hpp"

namespace uilo {

void Row::update(Bounds& parentBounds, float dt) {
    resize(parentBounds);

    // Group visible children by horizontal alignment and sum widths
    std::vector<Element*> lft, mid, rgt;
    float totalFixed = 0.f, totalPct = 0.f;

    for (auto* child : m_children) {
        if (!child->getModifier().getVisible()) continue;

        Align align = child->getModifier().getAlign();
        if      (hasAlign(align, Align::RIGHT))    rgt.push_back(child);
        else if (hasAlign(align, Align::CENTER_X)) mid.push_back(child);
        else                                        lft.push_back(child);

        Dimension dim = child->getModifier().getWidth();
        if (dim.percent) totalPct   += dim.value;
        else             totalFixed += dim.value;
    }

    if (lft.empty() && mid.empty() && rgt.empty()) return;

    // Pct children share this slot width so each resolves to its fair share of remaining space
    const float remaining = m_bounds.size.x - totalFixed;
    const float pctSlotW  = (totalPct > 0.f) ? (remaining * 100.f / totalPct) : 0.f;

    // Actual pixel width a child occupies after resize
    auto resolvedW = [&](Element* e) -> float {
        Dimension dim = e->getModifier().getWidth();
        return dim.percent ? (dim.value / 100.f * pctSlotW) : dim.value;
    };

    // Slot size.x to feed into resize so it produces the correct resolved width
    auto slotSizeX = [&](Element* e) -> float {
        Dimension dim = e->getModifier().getWidth();
        return dim.percent ? pctSlotW : dim.value;
    };

    // Offset slot.position.x so the child always lands at cursorX regardless of its horizontal align flag
    auto slotPosX = [&](Element* e, float cursorX) -> float {
        float sw = slotSizeX(e), rw = resolvedW(e);
        Align align = e->getModifier().getAlign();
        if      (hasAlign(align, Align::RIGHT))    return cursorX - (sw - rw);
        else if (hasAlign(align, Align::CENTER_X)) return cursorX - (sw - rw) * 0.5f;
        return cursorX;
    };

    // Pre-compute group block widths to position mid and rgt groups
    float midW = 0.f, rgtW = 0.f;
    for (auto* e : mid) midW += resolvedW(e);
    for (auto* e : rgt) rgtW += resolvedW(e);

    float xPos = m_bounds.left();
    for (auto* e : lft) {
        Bounds slot;
        slot.position = { slotPosX(e, xPos), m_bounds.position.y };
        slot.size     = { slotSizeX(e), m_bounds.size.y };
        e->update(slot, dt);
        xPos += resolvedW(e);
    }

    xPos = m_bounds.left() + (m_bounds.size.x - midW) * 0.5f;
    for (auto* e : mid) {
        Bounds slot;
        slot.position = { slotPosX(e, xPos), m_bounds.position.y };
        slot.size     = { slotSizeX(e), m_bounds.size.y };
        e->update(slot, dt);
        xPos += resolvedW(e);
    }

    xPos = m_bounds.right() - rgtW;
    for (auto* e : rgt) {
        Bounds slot;
        slot.position = { slotPosX(e, xPos), m_bounds.position.y };
        slot.size     = { slotSizeX(e), m_bounds.size.y };
        e->update(slot, dt);
        xPos += resolvedW(e);
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
