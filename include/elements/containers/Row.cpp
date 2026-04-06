#include "Row.hpp"

namespace uilo {

void Row::update(Bounds& parentBounds, float dt) {
    pruneChildren();
    resize(parentBounds);

    std::vector<Element*> lft, mid, rgt;
    float totalFixed = 0.f;

    for (auto* child : m_children) {
        if (!child->getModifier().getVisible()) continue;
        Align align = child->getModifier().getAlign();
        if      (hasAlign(align, Align::RIGHT))    rgt.push_back(child);
        else if (hasAlign(align, Align::CENTER_X)) mid.push_back(child);
        else                                       lft.push_back(child);
        Dimension dim = child->getModifier().getWidth();
        if (!dim.percent) totalFixed += dim.value;
    }

    const float remaining = m_bounds.size.x - totalFixed;
    const float pctSlotW  = remaining;

    auto resolvedW = [&](Element* e) -> float {
        Dimension dim = e->getModifier().getWidth();
        return dim.percent ? (dim.value / 100.f * pctSlotW) : dim.value;
    };
    auto slotSizeX = [&](Element* e) -> float {
        Dimension dim = e->getModifier().getWidth();
        return dim.percent ? pctSlotW : dim.value;
    };

    float midW = 0.f, rgtW = 0.f;
    for (auto* e : mid) midW += resolvedW(e);
    for (auto* e : rgt) rgtW += resolvedW(e);

    auto layoutGroup = [&](std::vector<Element*>& group, float startX) {
        float cursorX = startX;
        for (auto* child : group) {
            float sw = slotSizeX(child);
            float rw = resolvedW(child);
            Align align = child->getModifier().getAlign();
            // Adjust slot start so that after resize() applies horizontal alignment
            // the element lands exactly at cursorX (avoids double-centering pct slots).
            float slotX;
            if      (hasAlign(align, Align::RIGHT))    slotX = cursorX + rw - sw;
            else if (hasAlign(align, Align::CENTER_X)) slotX = cursorX - (sw - rw) * 0.5f;
            else                                       slotX = cursorX;
            Bounds slot;
            slot.position = { slotX, m_bounds.position.y };
            slot.size     = { sw, m_bounds.size.y };
            child->update(slot, dt);
            cursorX += rw;
        }
    };

    layoutGroup(lft, m_bounds.left());
    layoutGroup(mid, m_bounds.left() + (m_bounds.size.x - midW) * 0.5f);
    layoutGroup(rgt, m_bounds.right() - rgtW);
}

void Row::render(Renderer& renderer) {
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
