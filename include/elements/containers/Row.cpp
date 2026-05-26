#include "Row.hpp"
#include "../../UILO.hpp"
#include "../../utils/RenderUtils.hpp"
#include "../interactible/Resizer.hpp"
#include <algorithm>

namespace uilo {

Row::Row(Modifier modifier, RowOptions options, contains children, const std::string& name)
    : Container(modifier, children, name), m_options(options)
{}

void Row::update(sf::FloatRect& parentBounds, float dt) {
    pruneChildren();
    resize(parentBounds);

    float scale = m_uiloRef ? m_uiloRef->getScale() : 1.f;
    if (scale != m_lastScale && m_lastScale > 0.f) {
        m_scrollOffset *= scale / m_lastScale;
        m_lastScale = scale;
    }

    // Scrollable path: simple left-to-right stack, no alignment bucketing
    if (m_options.getScrollable()) {
        float cursorX = m_bounds.position.x - m_scrollOffset;
        m_contentWidth = 0.f;
        for (auto* child : m_children) {
            if (!child->getModifier().getVisible()) continue;
            if (child->getType() == ElementType::Resizer) continue;
            Dimension dim = child->getModifier().getWidth();
            float rw = dim.percent ? (m_bounds.size.x * dim.value / 100.f) : dim.value * scale;
            sf::FloatRect slot{ {cursorX, m_bounds.position.y}, {rw, m_bounds.size.y} };
            child->update(slot, dt);
            cursorX       += rw;
            m_contentWidth += rw;
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

            sf::FloatRect slot;
            slot.position   = { slotX, m_bounds.position.y };
            slot.size       = { sw, m_bounds.size.y };
            child->update(slot, dt);
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
        float boundX = (rEdge + lEdge) * 0.5f;

        // Clamp Y extent to the intersection of the adjacent panels' bounds
        // so the resizer doesn't bleed into areas outside the visible panels.
        float topY = std::max(
            prevEl ? prevEl->getBounds().position.y : m_bounds.position.y,
            nextEl ? nextEl->getBounds().position.y : m_bounds.position.y
        );
        float botY = std::min(
            prevEl ? (prevEl->getBounds().position.y + prevEl->getBounds().size.y) : (m_bounds.position.y + m_bounds.size.y),
            nextEl ? (nextEl->getBounds().position.y + nextEl->getBounds().size.y) : (m_bounds.position.y + m_bounds.size.y)
        );

        // Hit area uses the modifier's width (scaled), matching how every other
        // element scales. getThickness() is the visual strip width only.
        Dimension hitDim = r->getModifier().getWidth();
        float hitW = hitDim.percent ? (m_bounds.size.x * hitDim.value / 100.f)
                                    : hitDim.value * scale;
        sf::FloatRect rBounds = {
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
        child->update(rBounds, dt);
    }
}

void Row::render(sf::RenderTarget& target) {
    if (m_bounds.size.x <= 0.f || m_bounds.size.y <= 0.f) return;

    float scale = m_uiloRef ? m_uiloRef->getScale() : 1.f;
    const float r = m_options.getRounding() * scale;

    const sf::Vector2u needed{
        static_cast<unsigned int>(std::ceil(m_bounds.size.x)),
        static_cast<unsigned int>(std::ceil(m_bounds.size.y))
    };
    if (m_rt.getSize() != needed) {
        if (!m_rt.resize(needed)) return;
        m_dirty = true;
    }

    // Check if any visible child has changed content (not just position)
    if (!m_dirty) {
        for (auto* child : m_children) {
            if (child->getType() == ElementType::Resizer) continue;
            if (!m_bounds.findIntersection(child->getBounds())) continue;
            if (child->isDirty()) { m_dirty = true; break; }
        }
    }

    if (!m_dirty) {
        // Blit cached sprite at current position (handles moves without re-render)
        sf::Sprite sprite(m_rt.getTexture());
        sprite.setPosition(m_bounds.position);
        target.draw(sprite);
        return;
    }

    // Full re-render to RT
    m_rt.setView(sf::View(sf::FloatRect{m_bounds.position, m_bounds.size}));
    m_rt.clear(sf::Color::Transparent);

    sf::Color c = m_options.getColor();
    if (c.a > 0) {
        sf::RectangleShape bg(m_bounds.size);
        bg.setPosition(m_bounds.position);
        bg.setFillColor(c);
        m_rt.draw(bg);
    }

    for (auto* child : m_children) {
        if (child->getType() == ElementType::Resizer) continue;
        if (!m_bounds.findIntersection(child->getBounds())) continue;
        child->render(m_rt);
    }

    if (r > 0.f) eraseCorners(m_rt, m_bounds, r);
    m_rt.display();

    sf::Sprite sprite(m_rt.getTexture());
    sprite.setPosition(m_bounds.position);
    target.draw(sprite);

    m_dirty = false;
}

bool Row::checkScroll(const sf::Vector2f& mousePosition, float delta) {
    if (!m_bounds.contains(mousePosition)) return false;

    if (m_options.getScrollable()) {
        // Let nested scrollables consume first
        for (auto* child : m_children)
            if (child->getBounds().contains(mousePosition))
                if (child->checkScroll(mousePosition, delta)) return true;

        float maxScroll = std::max(0.f, m_contentWidth - m_bounds.size.x);
        m_scrollOffset = std::clamp(m_scrollOffset - delta * m_options.getScrollSpeed(),
                                    0.f, maxScroll);
        m_dirty = true;
        return true;
    }

    return Container::checkScroll(mousePosition, delta);
}

}