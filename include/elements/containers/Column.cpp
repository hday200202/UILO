#include "Column.hpp"
#include "../../UILO.hpp"
#include "../../utils/RenderUtils.hpp"
#include <algorithm>

namespace uilo {

Column::Column(Modifier modifier, ColumnOptions options, contains children, const std::string& name)
    : Container(modifier, children, name), m_options(options)
{}

void Column::update(sf::FloatRect& parentBounds, float dt) {
    pruneChildren();
    resize(parentBounds);

    float scale = m_uiloRef ? m_uiloRef->getScale() : 1.f;
    if (scale != m_lastScale && m_lastScale > 0.f) {
        m_scrollOffset *= scale / m_lastScale;
        m_lastScale = scale;
    }

    // Scrollable path: simple top-to-bottom stack, no alignment bucketing
    if (m_options.getScrollable()) {
        float cursorY = m_bounds.position.y - m_scrollOffset;
        m_contentHeight = 0.f;
        for (auto* child : m_children) {
            if (!child->getModifier().getVisible()) continue;
            Dimension dim = child->getModifier().getHeight();
            float rh = dim.percent ? (m_bounds.size.y * dim.value / 100.f) : dim.value * scale;
            sf::FloatRect slot{ {m_bounds.position.x, cursorY}, {m_bounds.size.x, rh} };
            child->update(slot, dt);
            cursorY      += rh;
            m_contentHeight += rh;
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

            sf::FloatRect slot;
            slot.position   = { m_bounds.position.x, slotY };
            slot.size       = { m_bounds.size.x, sh};
            child->update(slot, dt);
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
}

void Column::render(sf::RenderTarget& target) {
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

bool Column::checkScroll(const sf::Vector2f& mousePosition, float delta) {
    if (!m_bounds.contains(mousePosition)) return false;

    if (m_options.getScrollable()) {
        // Let nested scrollables consume first
        for (auto* child : m_children)
            if (child->getBounds().contains(mousePosition))
                if (child->checkScroll(mousePosition, delta)) return true;

        float maxScroll = std::max(0.f, m_contentHeight - m_bounds.size.y);
        m_scrollOffset = std::clamp(m_scrollOffset - delta * m_options.getScrollSpeed(),
                                    0.f, maxScroll);
        return true;
    }

    return Container::checkScroll(mousePosition, delta);
}

}