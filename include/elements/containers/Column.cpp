#include "Column.hpp"
#include "../../UILO.hpp"
#include "../../utils/RenderUtils.hpp"
#include "../interactible/Resizer.hpp"
#include <algorithm>
#include <cmath>

namespace uilo {

Column::Column(Modifier modifier, ColumnOptions options, contains children, const std::string& name)
    : Container(modifier, children, name), m_options(options)
{}

void Column::setScrollOffset(float offset) {
    const float maxScroll = std::max(0.f, m_contentHeight - m_bounds.size.y);
    m_scrollOffset = std::clamp(offset, 0.f, std::max(0.f, maxScroll));
    m_dirty = true;
}

void Column::update(Rectf& parentBounds, float dt) {
    pruneChildren();
    resize(parentBounds);
    const bool forceTreeUpdate = m_uiloRef && m_uiloRef->isForcingTreeUpdate();

    float scale = m_uiloRef ? m_uiloRef->getScale() : 1.f;
    if (scale != m_lastScale && m_lastScale > 0.f) {
        m_scrollOffset *= scale / m_lastScale;
        m_lastScale = scale;
    }

    m_scrollViewportHeight = m_bounds.size.y;

    // Scrollable path: supports pinned children (Modifier::ignoreScroll)
    // and scrolls only the remaining sibling viewport.
    if (m_options.getScrollable()) {
        std::vector<Element*> pinnedTop;
        std::vector<Element*> pinnedMid;
        std::vector<Element*> pinnedBottom;

        auto resolvedPinnedH = [&](Element* e) -> float {
            Dimension dim = e->getModifier().getHeight();
            return dim.percent ? (m_bounds.size.y * dim.value / 100.f) : dim.value * scale;
        };

        float pinnedTopH = 0.f;
        float pinnedBottomH = 0.f;
        float pinnedMidH = 0.f;

        for (auto* child : m_children) {
            if (!child->getModifier().getVisible()) continue;
            if (child->getType() == ElementType::Resizer) continue;
            if (!child->getModifier().getIgnoreScroll()) continue;

            const float rh = resolvedPinnedH(child);
            Align align = child->getModifier().getAlign();
            if (hasAlign(align, Align::Bottom)) {
                pinnedBottom.push_back(child);
                pinnedBottomH += rh;
            } else if (hasAlign(align, Align::CenterY)) {
                pinnedMid.push_back(child);
                pinnedMidH += rh;
            } else {
                pinnedTop.push_back(child);
                pinnedTopH += rh;
            }
        }

        auto layoutPinnedGroup = [&](std::vector<Element*>& group, float startY) {
            float cursorY = startY;
            for (auto* child : group) {
                const float rh = resolvedPinnedH(child);
                Rectf slot{{m_bounds.position.x, cursorY}, {m_bounds.size.x, rh}};
                child->tick(slot, dt);
                cursorY += rh;
            }
        };

        layoutPinnedGroup(pinnedTop, m_bounds.position.y);
        layoutPinnedGroup(pinnedMid, m_bounds.position.y + (m_bounds.size.y - pinnedMidH) * 0.5f);
        layoutPinnedGroup(pinnedBottom, m_bounds.position.y + m_bounds.size.y - pinnedBottomH);

        Rectf scrollViewport = m_bounds;
        scrollViewport.position.y += pinnedTopH;
        scrollViewport.size.y = std::max(0.f, m_bounds.size.y - pinnedTopH - pinnedBottomH);
        m_scrollViewportHeight = scrollViewport.size.y;

        float cursorY = scrollViewport.position.y - m_scrollOffset;
        m_contentHeight = 0.f;
        for (auto* child : m_children) {
            if (!child->getModifier().getVisible()) continue;
            if (child->getType() == ElementType::Resizer) continue;
            if (child->getModifier().getIgnoreScroll()) continue;

            Dimension dim = child->getModifier().getHeight();
            float rh = dim.percent ? (scrollViewport.size.y * dim.value / 100.f) : dim.value * scale;
            Rectf slot{ {scrollViewport.position.x, cursorY}, {scrollViewport.size.x, rh} };
            child->tick(slot, dt);
            cursorY      += rh;
            m_contentHeight += rh;
        }
        const float maxScroll = std::max(0.f, m_contentHeight - scrollViewport.size.y);
        if (m_scrollOffset > maxScroll) { m_scrollOffset = maxScroll; m_dirty = true; }
        if (m_scrollOffset < 0.f)        { m_scrollOffset = 0.f;        m_dirty = true; }

        // Keep resizers interactive in scrollable columns while still excluding
        // them from layout flow.
        for (size_t i = 0; i < m_children.size(); ++i) {
            auto* child = m_children[i];
            if (child->getType() != ElementType::Resizer) continue;
            Resizer* r = static_cast<Resizer*>(child);

            Element* prevEl = nullptr;
            for (int j = (int)i - 1; j >= 0; --j) {
                if (m_children[j]->getType() != ElementType::Resizer &&
                    m_children[j]->getModifier().getVisible()) { prevEl = m_children[j]; break; }
            }
            Element* nextEl = nullptr;
            for (size_t j = i + 1; j < m_children.size(); ++j) {
                if (m_children[j]->getType() != ElementType::Resizer &&
                    m_children[j]->getModifier().getVisible()) { nextEl = m_children[j]; break; }
            }

            float bEdge = prevEl ? (prevEl->getBounds().position.y + prevEl->getBounds().size.y
                                    + prevEl->getModifier().getOuterPadding() * scale)
                                 : m_bounds.position.y;
            float tEdge = nextEl ? (nextEl->getBounds().position.y
                                    - nextEl->getModifier().getOuterPadding() * scale)
                                 : (m_bounds.position.y + m_bounds.size.y);

            const bool hasPrev = (prevEl != nullptr);
            const bool hasNext = (nextEl != nullptr);
            float boundY = hasPrev && hasNext ? (bEdge + tEdge) * 0.5f
                                              : (hasPrev ? bEdge : (hasNext ? tEdge : (m_bounds.position.y + m_bounds.size.y * 0.5f)));

            float lftX = 0.f;
            float rgtX = 0.f;
            if (hasPrev && hasNext) {
                lftX = std::max(prevEl->getBounds().position.x, nextEl->getBounds().position.x);
                rgtX = std::min(prevEl->getBounds().position.x + prevEl->getBounds().size.x,
                                nextEl->getBounds().position.x + nextEl->getBounds().size.x);
            } else if (hasPrev) {
                lftX = prevEl->getBounds().position.x;
                rgtX = prevEl->getBounds().position.x + prevEl->getBounds().size.x;
            } else if (hasNext) {
                lftX = nextEl->getBounds().position.x;
                rgtX = nextEl->getBounds().position.x + nextEl->getBounds().size.x;
            } else {
                lftX = m_bounds.position.x;
                rgtX = m_bounds.position.x + m_bounds.size.x;
            }

            Dimension hitDim = r->getModifier().getHeight();
            float hitH = hitDim.percent ? (m_bounds.size.y * hitDim.value / 100.f)
                                        : hitDim.value * scale;
            const float spanW = std::max(0.f, rgtX - lftX);
            const Dimension widthDim = r->getModifier().getWidth();
            const float requestedW = widthDim.percent ? (spanW * widthDim.value / 100.f)
                                                      : widthDim.value * scale;
            const float hitW = std::clamp(requestedW, 0.f, spanW);
            const Align hitAlign = r->getModifier().getAlign();
            float hitX = lftX;
            if (hasAlign(hitAlign, Align::Right)) {
                hitX = rgtX - hitW;
            } else if (hasAlign(hitAlign, Align::CenterX)) {
                hitX = lftX + (spanW - hitW) * 0.5f;
            }
            Rectf rBounds = {
                { hitX, boundY - hitH * 0.5f },
                { hitW, hitH }
            };

            Element* resizerTarget = nullptr;
            switch (r->getDirection()) {
                case ResizerDir::Left:
                case ResizerDir::Top:    resizerTarget = prevEl; break;
                case ResizerDir::Right:
                case ResizerDir::Bottom: resizerTarget = nextEl; break;
            }
            r->setTarget(resizerTarget);
            r->setContainerBounds(m_bounds);
            child->tick(rBounds, dt);
        }

        if (forceTreeUpdate) {
            for (auto* child : m_children) {
                if (child->getType() == ElementType::Resizer) continue;
                if (child->getModifier().getVisible()) continue;
                Rectf slot = child->getBounds();
                if (slot.size.x <= 0.f || slot.size.y <= 0.f) {
                    slot.position = m_bounds.position;
                    slot.size = {0.f, 0.f};
                }
                child->tick(slot, dt);
            }
        }
        return;
    }

    std::vector<Element*> top;
    std::vector<Element*> mid;
    std::vector<Element*> bot;

    float totalFixed = 0.f;
    float totalPct = 0.f;

    /*
        First Pass:
        - Skip if not visible
        - Sort into it's corresponding bucket based on alignment
        - Add it's height (px or pct) to the totalFixed or totalPct
          depending on how it's height is calculated
    */
    for (auto* child : m_children) {
        if (!child->getModifier().getVisible()) continue;
        if (child->getType() == ElementType::Resizer) continue;

        Align align = child->getModifier().getAlign();
        if      (hasAlign(align, Align::Bottom))    bot.push_back(child);
        else if (hasAlign(align, Align::CenterY))   mid.push_back(child);
        else                                        top.push_back(child);

        Dimension dim = child->getModifier().getHeight();
        if (!dim.percent) totalFixed += dim.value * scale;
        else totalPct += dim.value;
    }

    /*
        Space Distribution:
        - Calculate how much vertical space remains after fixed-height children
        - Derive pctSlotH: the pixel height of a single percent-unit slot
    */
    const float remaining   = m_bounds.size.y - totalFixed;
    const float pctSlotH    = totalPct > 0.f ? (remaining * 100.f / totalPct) : remaining;

    /*
        Resolvers:
        - resolvedH: actual rendered pixel height of a child
        - slotSizeY: slot height used to advance the cursor
          (percent elements all share the same pctSlotH slot)
    */
    auto resolvedH = [&](Element* e) -> float {
        Dimension dim = e->getModifier().getHeight();
        return dim.percent ? (dim.value / 100.f * pctSlotH) : dim.value * scale;
    };

    auto slotSizeY = [&](Element* e) -> float {
        Dimension dim = e->getModifier().getHeight();
        return dim.percent ? pctSlotH : dim.value * scale;
    };

    /*
        Group Metrics:
        - Accumulate the total rendered height of mid and bot groups
          so their starting positions can be computed for centering/anchoring
    */
    float midH = 0.f;
    float botH = 0.f;

    for (auto* e : mid) midH += resolvedH(e);
    for (auto* e : bot) botH += resolvedH(e);

    /*
        Layout Group:
        - Places each child in a vertical slot starting at startY
        - Adjusts the slot origin within the slot based on the child's
          vertical alignment flag (TOP, MIDY, BOTTOM)
    */
    auto layoutGroup = [&](std::vector<Element*>& group, float startY) {
        float cursorY = startY;

        for (auto* child : group) {
            float sh = slotSizeY(child);
            float rh = resolvedH(child);

            Align align = child->getModifier().getAlign();

            float slotY;
            if      (hasAlign(align, Align::Bottom))    slotY = cursorY + rh - sh;
            else if (hasAlign(align, Align::CenterY))   slotY = cursorY - (sh - rh) * 0.5f;
            else                                        slotY = cursorY;

            Rectf slot;
            slot.position   = { m_bounds.position.x, slotY };
            slot.size       = { m_bounds.size.x, sh};
            child->tick(slot, dt);
            cursorY += rh;
        }
    };

    /*
        Second Pass:
        - Lay out top group from the top of m_bounds
        - Lay out mid group centered vertically within m_bounds
        - Lay out bot group anchored to the bottom of m_bounds
    */
    layoutGroup(top, m_bounds.position.y);
    layoutGroup(mid, m_bounds.position.y + (m_bounds.size.y - midH) * 0.5f);
    layoutGroup(bot, m_bounds.position.y + m_bounds.size.y - botH);

    // Position Resizer children at element boundaries (invisible to layout flow)
    for (size_t i = 0; i < m_children.size(); ++i) {
        auto* child = m_children[i];
        if (child->getType() != ElementType::Resizer) continue;
        Resizer* r = static_cast<Resizer*>(child);

        Element* prevEl = nullptr;
        for (int j = (int)i - 1; j >= 0; --j) {
            if (m_children[j]->getType() != ElementType::Resizer &&
                m_children[j]->getModifier().getVisible()) { prevEl = m_children[j]; break; }
        }
        Element* nextEl = nullptr;
        for (size_t j = i + 1; j < m_children.size(); ++j) {
            if (m_children[j]->getType() != ElementType::Resizer &&
                m_children[j]->getModifier().getVisible()) { nextEl = m_children[j]; break; }
        }

        // Use slot boundaries (restore outer padding) so asymmetric padding
        // between adjacent panels doesn't shift the center of the resizer.
        float bEdge = prevEl ? (prevEl->getBounds().position.y + prevEl->getBounds().size.y
                                + prevEl->getModifier().getOuterPadding() * scale)
                             : m_bounds.position.y;
        float tEdge = nextEl ? (nextEl->getBounds().position.y
                                - nextEl->getModifier().getOuterPadding() * scale)
                             : (m_bounds.position.y + m_bounds.size.y);

        const bool hasPrev = (prevEl != nullptr);
        const bool hasNext = (nextEl != nullptr);
        float boundY = hasPrev && hasNext ? (bEdge + tEdge) * 0.5f
                                          : (hasPrev ? bEdge : (hasNext ? tEdge : (m_bounds.position.y + m_bounds.size.y * 0.5f)));

        // Clamp X extent to adjacent panels when both exist; otherwise use the
        // single available panel so end-of-list resizers remain usable.
        float lftX = 0.f;
        float rgtX = 0.f;
        if (hasPrev && hasNext) {
            lftX = std::max(prevEl->getBounds().position.x, nextEl->getBounds().position.x);
            rgtX = std::min(prevEl->getBounds().position.x + prevEl->getBounds().size.x,
                            nextEl->getBounds().position.x + nextEl->getBounds().size.x);
        } else if (hasPrev) {
            lftX = prevEl->getBounds().position.x;
            rgtX = prevEl->getBounds().position.x + prevEl->getBounds().size.x;
        } else if (hasNext) {
            lftX = nextEl->getBounds().position.x;
            rgtX = nextEl->getBounds().position.x + nextEl->getBounds().size.x;
        } else {
            lftX = m_bounds.position.x;
            rgtX = m_bounds.position.x + m_bounds.size.x;
        }

        // Hit area uses the modifier's height (scaled), matching how every other
        // element scales. getThickness() is the visual strip height only.
        Dimension hitDim = r->getModifier().getHeight();
        float hitH = hitDim.percent ? (m_bounds.size.y * hitDim.value / 100.f)
                                    : hitDim.value * scale;
        const float spanW = std::max(0.f, rgtX - lftX);
        const Dimension widthDim = r->getModifier().getWidth();
        const float requestedW = widthDim.percent ? (spanW * widthDim.value / 100.f)
                                                  : widthDim.value * scale;
        const float hitW = std::clamp(requestedW, 0.f, spanW);
        const Align hitAlign = r->getModifier().getAlign();
        float hitX = lftX;
        if (hasAlign(hitAlign, Align::Right)) {
            hitX = rgtX - hitW;
        } else if (hasAlign(hitAlign, Align::CenterX)) {
            hitX = lftX + (spanW - hitW) * 0.5f;
        }
        Rectf rBounds = {
            { hitX, boundY - hitH * 0.5f },
            { hitW, hitH }
        };

        Element* resizerTarget = nullptr;
        switch (r->getDirection()) {
            case ResizerDir::Left:
            case ResizerDir::Top:    resizerTarget = prevEl; break;
            case ResizerDir::Right:
            case ResizerDir::Bottom: resizerTarget = nextEl; break;
        }
        r->setTarget(resizerTarget);
        r->setContainerBounds(m_bounds);
        child->tick(rBounds, dt);
    }

    if (forceTreeUpdate) {
        for (auto* child : m_children) {
            if (child->getType() == ElementType::Resizer) continue;
            if (child->getModifier().getVisible()) continue;
            Rectf slot = child->getBounds();
            if (slot.size.x <= 0.f || slot.size.y <= 0.f) {
                slot.position = m_bounds.position;
                slot.size = {0.f, 0.f};
            }
            child->tick(slot, dt);
        }
    }
}

void Column::render() {
    if (!m_modifier.getVisible()) return;
    if (m_bounds.size.x <= 0.f || m_bounds.size.y <= 0.f) return;

    // TODO: BGFX rendering — background fill + children
    // For now: unconditionally render all visible children
    float scale = m_uiloRef ? m_uiloRef->getScale() : 1.f;
    (void)scale;

    if (m_uiloRef) {
        const Color bg = m_uiloRef->getPalette().resolve(
            m_options.getColorRole(), m_options.getColor());
        const Material& mat = m_modifier.getMaterial();
        if (mat.kind != Material::Kind::None) {
            m_uiloRef->getRenderer().drawGlass(m_bounds, mat, bg);
        } else {
            if (bg.a > 0) {
                float r = m_options.getRounding() * scale;
                if (r <= 0.f)
                    m_uiloRef->getRenderer().draw(Rect{m_bounds.position, m_bounds.size, bg});
                else
                    m_uiloRef->getRenderer().draw(RoundedRect{m_bounds.position, m_bounds.size, r, 8u, bg});
            }
        }
    }

    const bool glassSubtree = m_uiloRef
        && m_modifier.getMaterial().kind != Material::Kind::None;
    if (glassSubtree) m_uiloRef->getRenderer().beginGlassSubtree();

    auto renderPass = [&](bool ignoreScrollChildren) {
        for (auto* child : m_children) {
            if (child->getType() == ElementType::Resizer) continue;
            if (child->getModifier().getIgnoreScroll() != ignoreScrollChildren) continue;
            if (m_uiloRef) {
                const float rr = m_options.getRounding() * (m_uiloRef->getScale());
                m_uiloRef->getRenderer().pushRoundClip(m_bounds, rr);
            }
            child->render();
            if (m_uiloRef) m_uiloRef->getRenderer().popRoundClip();
        }
    };

    // Base scrolling content first, pinned ignoreScroll content last on top.
    renderPass(false);
    renderPass(true);

    if (glassSubtree) m_uiloRef->getRenderer().endGlassSubtree();

    m_dirty = false;
}

bool Column::checkScroll(const Vec2f& mousePosition, float delta, bool precise, bool momentum) {
    if (!m_bounds.contains(mousePosition)) return false;

    if (m_options.getScrollable()) {
        for (auto* child : m_children)
            if (child->getBounds().contains(mousePosition))
                if (child->checkScroll(mousePosition, delta, precise, momentum)) return true;

        // Trackpad: pixel-precise per-event (OS already provides momentum tail).
        // Wheel: discrete step. ScrollSpeed scales both (40 = default 1:1).
        const float speed     = m_options.getScrollSpeed();
        const float step      = precise ? 30.f * (speed / 40.f) : speed;
        const float maxScroll = std::max(0.f, m_contentHeight - m_scrollViewportHeight);
        if (maxScroll <= 0.f) return false;
        m_scrollOffset = std::clamp(m_scrollOffset - delta * step, 0.f, maxScroll);
        m_dirty = true;
        return true;
    }

    return Container::checkScroll(mousePosition, delta, precise, momentum);
}

bool Column::checkScroll(const Vec2f& mousePosition, Vec2f delta, bool precise, bool momentum) {
    if (!m_bounds.contains(mousePosition)) return false;

    // Try children with the full 2D delta first so a 2-axis consumer like
    // Canvas can grab both components before we use delta.y for our own
    // vertical scroll behavior.
    for (auto* child : m_children)
        if (child->getBounds().contains(mousePosition))
            if (child->checkScroll(mousePosition, delta, precise, momentum)) return true;

    if (m_options.getScrollable()) {
        const float speed     = m_options.getScrollSpeed();
        const float step      = precise ? 30.f * (speed / 40.f) : speed;
        const float maxScroll = std::max(0.f, m_contentHeight - m_scrollViewportHeight);
        if (maxScroll <= 0.f) return false;
        m_scrollOffset = std::clamp(m_scrollOffset - delta.y * step, 0.f, maxScroll);
        m_dirty = true;
        return true;
    }

    if (m_modifier.getOnScroll()) {
        m_modifier.getOnScroll()(this, delta.y);
        return true;
    }

    return false;
}

}