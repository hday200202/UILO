#include "Row.hpp"
#include "../../UILO.hpp"
#include "../../utils/RenderUtils.hpp"
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
}

void Row::render(sf::RenderTarget& target) {
    float scale = m_uiloRef ? m_uiloRef->getScale() : 1.f;
    const float r = m_options.getRounding() * scale;

    if (r <= 0.f) {
        const sf::View originalView = target.getView();
        const sf::Vector2u winSize  = target.getSize();

        sf::View clipView;
        clipView.setCenter(m_bounds.position + m_bounds.size / 2.f);
        clipView.setSize(m_bounds.size);
        clipView.setViewport(sf::FloatRect{
            { m_bounds.position.x / winSize.x, m_bounds.position.y / winSize.y },
            { m_bounds.size.x     / winSize.x, m_bounds.size.y     / winSize.y }
        });

        target.setView(clipView);

        sf::Color c = m_options.getColor();
        if (c.a > 0) {
            sf::RectangleShape rect;
            rect.setPosition(m_bounds.position);
            rect.setSize(m_bounds.size);
            rect.setFillColor(c);
            target.draw(rect);
        }

        for (auto* child : m_children) child->render(target);

        target.setView(originalView);
    } else {
        // RenderTexture path: draw into window-sized RT then erase corners
        const auto winSize = target.getSize();
        if (m_rt.getSize() != winSize) {
            if (!m_rt.resize(winSize)) return;
        }

        sf::View fullView(sf::FloatRect{
            {0.f, 0.f},
            {static_cast<float>(winSize.x), static_cast<float>(winSize.y)}
        });
        m_rt.setView(fullView);
        m_rt.clear(sf::Color::Transparent);

        sf::Color c = m_options.getColor();
        if (c.a > 0) {
            sf::RectangleShape bg(m_bounds.size);
            bg.setPosition(m_bounds.position);
            bg.setFillColor(c);
            m_rt.draw(bg);
        }

        for (auto* child : m_children) child->render(m_rt);

        eraseCorners(m_rt, m_bounds, r);
        m_rt.display();

        sf::IntRect texRect{
            {static_cast<int>(m_bounds.position.x), static_cast<int>(m_bounds.position.y)},
            {static_cast<int>(m_bounds.size.x),     static_cast<int>(m_bounds.size.y)}
        };
        sf::Sprite sprite(m_rt.getTexture(), texRect);
        sprite.setPosition(m_bounds.position);
        target.draw(sprite);
    }
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
        return true;
    }

    return Container::checkScroll(mousePosition, delta);
}

}