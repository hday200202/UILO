#include "Row.hpp"
#include "../../UILO.hpp"
#include "../../utils/RenderUtils.hpp"
#include "../interactible/Resizer.hpp"
#include <algorithm>
#include <cmath>

namespace uilo {

Row::Row(Modifier modifier, RowOptions options, contains children, const std::string& name)
    : Container(modifier, children, name), m_options(options)
{}

void Row::update(Rectf& parentBounds, float dt) {
    pruneChildren();
    resize(parentBounds);
    const bool forceTreeUpdate = m_uiloRef && m_uiloRef->isForcingTreeUpdate();

    float scale = m_uiloRef ? m_uiloRef->getScale() : 1.f;
    if (scale != m_lastScale && m_lastScale > 0.f) {
        m_scrollOffset *= scale / m_lastScale;
        m_lastScale = scale;
    }

    m_scrollViewportWidth = m_bounds.size.x;

    // Scrollable path: supports pinned children (Modifier::ignoreScroll)
    // and scrolls only the remaining sibling viewport.
    if (m_options.getScrollable()) {
        std::vector<Element*> pinnedLeft;
        std::vector<Element*> pinnedMid;
        std::vector<Element*> pinnedRight;

        auto resolvedPinnedW = [&](Element* e) -> float {
            Dimension dim = e->getModifier().getWidth();
            return dim.percent ? (m_bounds.size.x * dim.value / 100.f) : dim.value * scale;
        };

        float pinnedLeftW = 0.f;
        float pinnedRightW = 0.f;
        float pinnedMidW = 0.f;

        for (auto* child : m_children) {
            if (!child->getModifier().getVisible()) continue;
            if (child->getType() == ElementType::Resizer) continue;
            if (!child->getModifier().getIgnoreScroll()) continue;

            const float rw = resolvedPinnedW(child);
            Align align = child->getModifier().getAlign();
            if (hasAlign(align, Align::Right)) {
                pinnedRight.push_back(child);
                pinnedRightW += rw;
            } else if (hasAlign(align, Align::CenterX)) {
                pinnedMid.push_back(child);
                pinnedMidW += rw;
            } else {
                pinnedLeft.push_back(child);
                pinnedLeftW += rw;
            }
        }

        auto layoutPinnedGroup = [&](std::vector<Element*>& group, float startX) {
            float cursorX = startX;
            for (auto* child : group) {
                const float rw = resolvedPinnedW(child);
                Rectf slot{{cursorX, m_bounds.position.y}, {rw, m_bounds.size.y}};
                child->tick(slot, dt);
                cursorX += rw;
            }
        };

        layoutPinnedGroup(pinnedLeft, m_bounds.position.x);
        layoutPinnedGroup(pinnedMid, m_bounds.position.x + (m_bounds.size.x - pinnedMidW) * 0.5f);
        layoutPinnedGroup(pinnedRight, m_bounds.position.x + m_bounds.size.x - pinnedRightW);

        Rectf scrollViewport = m_bounds;
        scrollViewport.position.x += pinnedLeftW;
        scrollViewport.size.x = std::max(0.f, m_bounds.size.x - pinnedLeftW - pinnedRightW);
        m_scrollViewportWidth = scrollViewport.size.x;

        float cursorX = scrollViewport.position.x - m_scrollOffset;
        m_contentWidth = 0.f;
        for (auto* child : m_children) {
            if (!child->getModifier().getVisible()) continue;
            if (child->getType() == ElementType::Resizer) continue;
            if (child->getModifier().getIgnoreScroll()) continue;

            Dimension dim = child->getModifier().getWidth();
            float rw = dim.percent ? (scrollViewport.size.x * dim.value / 100.f) : dim.value * scale;
            Rectf slot{ {cursorX, scrollViewport.position.y}, {rw, scrollViewport.size.y} };
            child->tick(slot, dt);
            cursorX       += rw;
            m_contentWidth += rw;
        }
        const float maxScroll = std::max(0.f, m_contentWidth - scrollViewport.size.x);
        if (m_scrollOffset > maxScroll) { m_scrollOffset = maxScroll; m_dirty = true; }
        if (m_scrollOffset < 0.f)        { m_scrollOffset = 0.f;        m_dirty = true; }

        // Keep resizers interactive in scrollable rows while still excluding
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

            float rEdge = prevEl ? (prevEl->getBounds().position.x + prevEl->getBounds().size.x
                                    + prevEl->getModifier().getOuterPadding() * scale)
                                 : m_bounds.position.x;
            float lEdge = nextEl ? (nextEl->getBounds().position.x
                                    - nextEl->getModifier().getOuterPadding() * scale)
                                 : (m_bounds.position.x + m_bounds.size.x);

            const bool hasPrev = (prevEl != nullptr);
            const bool hasNext = (nextEl != nullptr);
            float boundX = hasPrev && hasNext ? (rEdge + lEdge) * 0.5f
                                              : (hasPrev ? rEdge : (hasNext ? lEdge : (m_bounds.position.x + m_bounds.size.x * 0.5f)));

            float topY = 0.f;
            float botY = 0.f;
            if (hasPrev && hasNext) {
                topY = std::max(prevEl->getBounds().position.y, nextEl->getBounds().position.y);
                botY = std::min(prevEl->getBounds().position.y + prevEl->getBounds().size.y,
                                nextEl->getBounds().position.y + nextEl->getBounds().size.y);
            } else if (hasPrev) {
                topY = prevEl->getBounds().position.y;
                botY = prevEl->getBounds().position.y + prevEl->getBounds().size.y;
            } else if (hasNext) {
                topY = nextEl->getBounds().position.y;
                botY = nextEl->getBounds().position.y + nextEl->getBounds().size.y;
            } else {
                topY = m_bounds.position.y;
                botY = m_bounds.position.y + m_bounds.size.y;
            }

            Dimension hitDim = r->getModifier().getWidth();
            float hitW = hitDim.percent ? (m_bounds.size.x * hitDim.value / 100.f)
                                        : hitDim.value * scale;
            Rectf rBounds = {
                { boundX - hitW * 0.5f, topY },
                { hitW, std::max(0.f, botY - topY) }
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

    std::vector<Element*> left;
    std::vector<Element*> mid;
    std::vector<Element*> right;

    float totalFixed = 0.f;
    float totalPct = 0.f;

    /*
        First Pass:
        - Skip if not visible
        - Sort into its corresponding bucket based on alignment
        - Add its width (px or pct) to the totalFixed or totalPct
          depending on how its width is calculated
    */
    for (auto* child : m_children) {
        if (!child->getModifier().getVisible()) continue;
        if (child->getType() == ElementType::Resizer) continue;

        Align align = child->getModifier().getAlign();
        if      (hasAlign(align, Align::Right))     right.push_back(child);
        else if (hasAlign(align, Align::CenterX))   mid.push_back(child);
        else                                        left.push_back(child);

        Dimension dim = child->getModifier().getWidth();
        if (!dim.percent) totalFixed += dim.value * scale;
        else totalPct += dim.value;
    }

    /*
        Space Distribution:
        - Calculate how much horizontal space remains after fixed-width children
        - Derive pctSlotW: the pixel width of a single percent-unit slot
    */
    const float remaining   = m_bounds.size.x - totalFixed;
    const float pctSlotW    = totalPct > 0.f ? (remaining * 100.f / totalPct) : remaining;

    /*
        Resolvers:
        - resolvedW: actual rendered pixel width of a child
        - slotSizeX: slot width used to advance the cursor
          (percent elements all share the same pctSlotW slot)
    */
    auto resolvedW = [&](Element* e) -> float {
        Dimension dim = e->getModifier().getWidth();
        return dim.percent ? (dim.value / 100.f * pctSlotW) : dim.value * scale;
    };

    auto slotSizeX = [&](Element* e) -> float {
        Dimension dim = e->getModifier().getWidth();
        return dim.percent ? pctSlotW : dim.value * scale;
    };

    /*
        Group Metrics:
        - Accumulate the total rendered width of mid and right groups
          so their starting positions can be computed for centering/anchoring
    */
    float midW = 0.f;
    float rightW = 0.f;

    for (auto* e : mid)   midW   += resolvedW(e);
    for (auto* e : right) rightW += resolvedW(e);

    /*
        Layout Group:
        - Places each child in a horizontal slot starting at startX
        - Adjusts the slot origin within the slot based on the child's
          horizontal alignment flag (LEFT, MIDX, RIGHT)
    */
    auto layoutGroup = [&](std::vector<Element*>& group, float startX) {
        float cursorX = startX;

        for (auto* child : group) {
            float sw = slotSizeX(child);
            float rw = resolvedW(child);

            Align align = child->getModifier().getAlign();

            float slotX;
            if      (hasAlign(align, Align::Right))     slotX = cursorX + rw - sw;
            else if (hasAlign(align, Align::CenterX))   slotX = cursorX - (sw - rw) * 0.5f;
            else                                        slotX = cursorX;

            Rectf slot;
            slot.position   = { slotX, m_bounds.position.y };
            slot.size       = { sw, m_bounds.size.y };
            child->tick(slot, dt);
            cursorX += rw;
        }
    };

    /*
        Second Pass:
        - Lay out left group from the left edge of m_bounds
        - Lay out mid group centered horizontally within m_bounds
        - Lay out right group anchored to the right edge of m_bounds
    */
    layoutGroup(left,  m_bounds.position.x);
    layoutGroup(mid,   m_bounds.position.x + (m_bounds.size.x - midW) * 0.5f);
    layoutGroup(right, m_bounds.position.x + m_bounds.size.x - rightW);

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
        float rEdge = prevEl ? (prevEl->getBounds().position.x + prevEl->getBounds().size.x
                                + prevEl->getModifier().getOuterPadding() * scale)
                             : m_bounds.position.x;
        float lEdge = nextEl ? (nextEl->getBounds().position.x
                                - nextEl->getModifier().getOuterPadding() * scale)
                             : (m_bounds.position.x + m_bounds.size.x);

        const bool hasPrev = (prevEl != nullptr);
        const bool hasNext = (nextEl != nullptr);
        float boundX = hasPrev && hasNext ? (rEdge + lEdge) * 0.5f
                                          : (hasPrev ? rEdge : (hasNext ? lEdge : (m_bounds.position.x + m_bounds.size.x * 0.5f)));

        // Clamp Y extent to adjacent panels when both exist; otherwise use the
        // single available panel so end-of-list resizers remain usable.
        float topY = 0.f;
        float botY = 0.f;
        if (hasPrev && hasNext) {
            topY = std::max(prevEl->getBounds().position.y, nextEl->getBounds().position.y);
            botY = std::min(prevEl->getBounds().position.y + prevEl->getBounds().size.y,
                            nextEl->getBounds().position.y + nextEl->getBounds().size.y);
        } else if (hasPrev) {
            topY = prevEl->getBounds().position.y;
            botY = prevEl->getBounds().position.y + prevEl->getBounds().size.y;
        } else if (hasNext) {
            topY = nextEl->getBounds().position.y;
            botY = nextEl->getBounds().position.y + nextEl->getBounds().size.y;
        } else {
            topY = m_bounds.position.y;
            botY = m_bounds.position.y + m_bounds.size.y;
        }

        // Hit area uses the modifier's width (scaled), matching how every other
        // element scales. getThickness() is the visual strip width only.
        Dimension hitDim = r->getModifier().getWidth();
        float hitW = hitDim.percent ? (m_bounds.size.x * hitDim.value / 100.f)
                                    : hitDim.value * scale;
        Rectf rBounds = {
            { boundX - hitW * 0.5f, topY },
            { hitW, std::max(0.f, botY - topY) }
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

void Row::render() {
    if (!m_modifier.getVisible()) return;
    if (m_bounds.size.x <= 0.f || m_bounds.size.y <= 0.f) return;

    // TODO: BGFX rendering — background fill + children
    float scale = m_uiloRef ? m_uiloRef->getScale() : 1.f;
    (void)scale;

    if (m_uiloRef) {
        const Color bg = m_uiloRef->getPalette().resolve(
            m_options.getColorRole(), m_options.getColor());
        const Material& mat = m_modifier.getMaterial();
        if (mat.kind != Material::Kind::None) {
            // The material owns the background: fs_glass draws its own
            // rounded rect (radius from mat.cornerRadius), tint and per-kind
            // effect, so don't double-fill.
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

bool Row::checkScroll(const Vec2f& mousePosition, float delta, bool precise, bool momentum) {
    if (!m_bounds.contains(mousePosition)) return false;

    if (m_options.getScrollable()) {
        for (auto* child : m_children)
            if (child->getBounds().contains(mousePosition))
                if (child->checkScroll(mousePosition, delta, precise, momentum)) return true;

        const float speed     = m_options.getScrollSpeed();
        const float step      = precise ? 30.f * (speed / 40.f) : speed;
        const float maxScroll = std::max(0.f, m_contentWidth - m_scrollViewportWidth);
        if (maxScroll <= 0.f) return false;
        m_scrollOffset = std::clamp(m_scrollOffset - delta * step, 0.f, maxScroll);
        m_dirty = true;
        return true;
    }

    return Container::checkScroll(mousePosition, delta, precise, momentum);
}

bool Row::checkScroll(const Vec2f& mousePosition, Vec2f delta, bool precise, bool momentum) {
    if (!m_bounds.contains(mousePosition)) return false;

    for (auto* child : m_children)
        if (child->getBounds().contains(mousePosition))
            if (child->checkScroll(mousePosition, delta, precise, momentum)) return true;

    if (m_options.getScrollable()) {
        const float speed     = m_options.getScrollSpeed();
        const float step      = precise ? 30.f * (speed / 40.f) : speed;
        const float maxScroll = std::max(0.f, m_contentWidth - m_scrollViewportWidth);
        if (maxScroll <= 0.f) return false;
        // Row is horizontal; prefer delta.x but fall back to delta.y so a
        // plain vertical mouse wheel still works as the user expects.
        const float d = (delta.x != 0.f) ? delta.x : delta.y;
        m_scrollOffset = std::clamp(m_scrollOffset - d * step, 0.f, maxScroll);
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