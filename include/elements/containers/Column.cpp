#include "Column.hpp"
#include "../../UILO.hpp"
#include "../../utils/RenderUtils.hpp"
#include "../interactible/Resizer.hpp"
#include <algorithm>
#include <cmath>
#include <vector>

namespace uilo {

namespace {
/*
    resolveScrollBounds(...):
    - Params:   const ColumnOptions& options, float contentMax, float&
                minScroll, float& maxScroll
    - Returns:  void
    - Desc:     Works out how far a column may scroll. With both explicit bounds
                set the options win outright, swapped into order if they arrive
                reversed; otherwise the range runs from 0 to however much
                content overflows the viewport.
*/
void resolveScrollBounds(
    const ColumnOptions& options,
    float contentMax,
    float& minScroll,
    float& maxScroll
) {
    if (options.hasScrollMin() && options.hasScrollMax()) {
        minScroll = options.getScrollMin();
        maxScroll = options.getScrollMax();
        if (minScroll > maxScroll) std::swap(minScroll, maxScroll);
        return;
    }

    minScroll = 0.f;
    maxScroll = std::max(0.f, contentMax);
}


/*
    canUseLooseScrollBounds(const ColumnOptions& options, float contentMax):
    - Params:   const ColumnOptions& options, float contentMax
    - Returns:  bool
    - Desc:     Whether a scroll event should be consumed at all. Explicit
                bounds always allow it; otherwise there has to be overflow to
                scroll into, so a column whose content fits lets the event
                bubble to its parent.
*/
bool canUseLooseScrollBounds(const ColumnOptions& options, float contentMax) {
    return (options.hasScrollMin() && options.hasScrollMax()) || contentMax > 0.f;
}


/*
    normalizeGridStep(float step, float ratio, float minDistance, float maxDistance):
    - Params:   float step, float ratio, float minDistance, float maxDistance
    - Returns:  float
    - Desc:     Coarsens or refines a grid step by whole ratio steps until it
                lands between the two thresholds. This is what keeps the
                subdivision grid at a stable on-screen density while zooming,
                instead of the lines crowding together or drifting apart.
*/
float normalizeGridStep(
    float step,
    float ratio,
    float minDistance,
    float maxDistance
) {
    if (step <= 0.f || minDistance <= 0.f || maxDistance <= 0.f) return step;
    if (ratio <= 1.f) ratio = 2.f;
    if (maxDistance < minDistance) maxDistance = minDistance;

    while (step < minDistance) step *= ratio;
    while (step > maxDistance) step /= ratio;
    return step;
}
}


/*
    Column(...):
    - Params:   Modifier modifier, ColumnOptions options, contains children,
                const std::string& name
    - Returns:  Column
    - Desc:     Constructs a column from a modifier, its options and its
                children, and tags it as a Column so layout and hit-testing can
                identify it.
*/
Column::Column(
    Modifier modifier,
    ColumnOptions options,
    contains children,
    const std::string& name
) : Container(modifier, children, name), m_options(options) {
    m_type = ElementType::Column;
}


/*
    contentOverflow():
    - Params:   none
    - Returns:  float
    - Desc:     How much content sticks out past the scrolling viewport, never
                negative. This is the distance the column can travel when no
                explicit scroll bounds were set.
*/
float Column::contentOverflow() const {
    return std::max(0.f, m_contentHeight - m_scrollViewportHeight);
}


/*
    scrollStep(bool precise):
    - Params:   bool precise
    - Returns:  float
    - Desc:     Pixels to travel per unit of scroll delta. A trackpad reports a
                precise pixel delta and is scaled against scrollSpeed
                differently from a wheel's discrete step, since the OS already
                supplies the momentum tail.
*/
float Column::scrollStep(bool precise) const {
    const float speed = m_options.getScrollSpeed();
    return precise ? 30.f * (speed / 40.f) : speed;
}


/*
    readScrollLinks():
    - Params:   none
    - Returns:  void
    - Desc:     Takes the shared scroll offset and zoom from the link groups
                this column belongs to. Linked columns travel together, so the
                shared value has to be adopted before layout rather than after
                it.
*/
void Column::readScrollLinks() {
    if (!m_uiloRef) return;

    if (!m_options.getScrollLink().empty())
        m_scrollOffset = m_uiloRef->getScrollLinkOffset(m_options.getScrollLink(), false);

    if (!m_options.getZoomLink().empty())
        m_zoomY = m_uiloRef->getZoomLinkValue(m_options.getZoomLink(), false);
}


/*
    publishScrollLinks():
    - Params:   none
    - Returns:  void
    - Desc:     Writes this column's scroll offset and zoom back to its link
                groups, so every other column sharing the same id picks them up
                on its next update.
*/
void Column::publishScrollLinks() const {
    if (!m_uiloRef) return;

    if (!m_options.getScrollLink().empty())
        m_uiloRef->setScrollLinkOffset(m_options.getScrollLink(), m_scrollOffset, false);

    if (!m_options.getZoomLink().empty())
        m_uiloRef->setZoomLinkValue(m_options.getZoomLink(), m_zoomY, false);
}


/*
    applyScrollOffset(float target):
    - Params:   float target
    - Returns:  void
    - Desc:     Moves the column to an absolute offset, clamped into the range
                the current content allows, then publishes it to any linked
                column and marks the column dirty.
*/
void Column::applyScrollOffset(float target) {
    float minScroll = 0.f;
    float maxScroll = 0.f;
    resolveScrollBounds(m_options, contentOverflow(), minScroll, maxScroll);

    m_scrollOffset = std::clamp(target, minScroll, maxScroll);
    publishScrollLinks();
    m_dirty = true;
}


/*
    setScrollOffset(float offset):
    - Params:   float offset
    - Returns:  void
    - Desc:     Scrolls to an absolute offset, clamped to the range the current
                content allows. Used to drive a column from somewhere other than
                a scroll gesture. Measures the overflow against the column's own
                height rather than the scrolling viewport, so it can be called
                before the first layout pass has run.
*/
void Column::setScrollOffset(float offset) {
    const float contentMax = std::max(0.f, m_contentHeight - m_bounds.size.y);
    float minScroll = 0.f;
    float maxScroll = 0.f;
    resolveScrollBounds(m_options, contentMax, minScroll, maxScroll);
    m_scrollOffset = std::clamp(offset, minScroll, maxScroll);
    m_dirty = true;
}


/*
    update(Rectf& parentBounds, float dt):
    - Params:   Rectf& parentBounds, float dt
    - Returns:  void
    - Desc:     Lays the column's children out along the vertical axis and ticks
                each one with the slot it was given. Resolves the column's own
                bounds, adopts any linked scroll offset and zoom, then hands the
                children to whichever of the two layout paths applies:
                updateScrollable when the column scrolls, updateFlow when it
                does not. Resizers and hidden children are dealt with afterwards
                in both cases, since neither takes part in the flow.
*/
void Column::update(Rectf& parentBounds, float dt) {
    pruneChildren();
    resize(parentBounds);

    /* Keep the scroll offset proportional when the UI scale changes. */
    const float scale = m_uiloRef ? m_uiloRef->getScale() : 1.f;
    if (scale != m_lastScale && m_lastScale > 0.f) {
        m_scrollOffset *= scale / m_lastScale;
        m_lastScale = scale;
    }

    readScrollLinks();

    m_scrollViewportHeight = m_bounds.size.y;

    if (m_options.getScrollable()) updateScrollable(dt, scale);
    else                           updateFlow(dt, scale);

    layoutResizers(dt, scale);
    tickHiddenChildren(dt);

    /* Outside the flow, so placed after it and against the content area. */
    tickFloating(dt);
}


/*
    updateScrollable(float dt, float scale):
    - Params:   float dt, float scale
    - Returns:  void
    - Desc:     Lays out a scrolling column. Pinned children -- those with
                Modifier::ignoreScroll -- are placed first, grouped top, centre
                and bottom, and the height they take is reserved: whatever is
                left between the top and bottom strips becomes the scrolling
                viewport, so a pinned header stays put while the rest of the
                column travels. The scrolling children are then placed from the
                viewport's top edge displaced by the scroll offset, their total
                measured as the content height, and the offset finally clamped
                into the range that content allows and published to any linked
                column.
*/
void Column::updateScrollable(float dt, float scale) {
    /* Children live inside the inner padding; the container's own bounds do not move. */
    const Rectf area = contentArea();

    std::vector<Element*> pinnedTop;
    std::vector<Element*> pinnedMid;
    std::vector<Element*> pinnedBottom;

    auto resolvedPinnedH = [&](Element* e) -> float {
        Dimension dim = e->getModifier().getHeight();
        return dim.percent ? (area.size.y * dim.value / 100.f) : dim.value * scale;
    };

    float pinnedTopH = 0.f;
    float pinnedBottomH = 0.f;
    float pinnedMidH = 0.f;

    /* Sort the pinned children into top, centre and bottom groups. */
    for (auto* child : m_children) {
        if (!child->getModifier().getVisible()) continue;
        if (child->getType() == ElementType::Resizer) continue;
        if (child->getModifier().isFloating()) continue;
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
            Rectf slot{{area.position.x, cursorY}, {area.size.x, rh}};
            child->tick(slot, dt);
            cursorY += rh;
        }
    };

    layoutPinnedGroup(pinnedTop, area.position.y);
    layoutPinnedGroup(pinnedMid, area.position.y + (area.size.y - pinnedMidH) * 0.5f);
    layoutPinnedGroup(pinnedBottom, area.position.y + area.size.y - pinnedBottomH);

    /* Whatever the pinned strips did not take is the scrolling viewport. */
    Rectf scrollViewport = area;
    scrollViewport.position.y += pinnedTopH;
    scrollViewport.size.y = std::max(0.f, area.size.y - pinnedTopH - pinnedBottomH);
    m_scrollViewportHeight = scrollViewport.size.y;

    /* Lay the scrolling children out, displaced by the scroll offset. */
    float cursorY = scrollViewport.position.y - m_scrollOffset;
    m_contentHeight = 0.f;
    for (auto* child : m_children) {
        if (!child->getModifier().getVisible()) continue;
        if (child->getType() == ElementType::Resizer) continue;
        if (child->getModifier().isFloating()) continue;
        if (child->getModifier().getIgnoreScroll()) continue;

        Dimension dim = child->getModifier().getHeight();
        const float zf = m_options.getZoomableY() ? m_zoomY : 1.f;
        float rh = dim.percent ? (scrollViewport.size.y * dim.value / 100.f) : dim.value * scale * zf;
        Rectf slot{ {scrollViewport.position.x, cursorY}, {scrollViewport.size.x, rh} };
        child->tick(slot, dt);
        cursorY      += rh;
        m_contentHeight += rh;
    }

    /* Clamp into the resolved bounds and publish to any linked column. */
    m_scrollViewportY = scrollViewport.position.y;
    float minScroll = 0.f;
    float maxScroll = 0.f;
    resolveScrollBounds(m_options, contentOverflow(), minScroll, maxScroll);
    const float clamped = std::clamp(m_scrollOffset, minScroll, maxScroll);
    if (clamped != m_scrollOffset) { m_scrollOffset = clamped; m_dirty = true; }
    publishScrollLinks();
}


/*
    updateFlow(float dt, float scale):
    - Params:   float dt, float scale
    - Returns:  void
    - Desc:     Lays out a non-scrolling column. Children are sorted into top,
                centre and bottom buckets by their alignment, then the height
                left over after the fixed-size ones is shared among the percent-
                sized ones -- every percent child gets the same slot, so two 50%
                siblings beside a 100px one split what remains rather than the
                whole column. Each group is finally placed from its own anchor:
                top from the top edge, centre about the middle, bottom against
                the bottom edge.
*/
void Column::updateFlow(float dt, float scale) {
    /* Children live inside the inner padding; the container's own bounds do not move. */
    const Rectf area = contentArea();

    /* First pass: bucket the children by alignment, and total how much height is
       fixed against how many percent units are asking for the rest. */
    std::vector<Element*> top;
    std::vector<Element*> mid;
    std::vector<Element*> bot;

    float totalFixed = 0.f;
    float totalPct = 0.f;

    for (auto* child : m_children) {
        if (!child->getModifier().getVisible()) continue;
        if (child->getType() == ElementType::Resizer) continue;
        if (child->getModifier().isFloating()) continue;

        Align align = child->getModifier().getAlign();
        if      (hasAlign(align, Align::Bottom))    bot.push_back(child);
        else if (hasAlign(align, Align::CenterY))   mid.push_back(child);
        else                                        top.push_back(child);

        Dimension dim = child->getModifier().getHeight();
        if (!dim.percent) totalFixed += dim.value * scale;
        else totalPct += dim.value;
    }

    /* What is left after the fixed-height children, and the height of a single
       percent-unit slot. */
    const float remaining   = area.size.y - totalFixed;
    const float pctSlotH    = totalPct > 0.f ? (remaining * 100.f / totalPct) : remaining;

    /* resolvedH is what a child draws at; slotSizeY is what advances the cursor,
       and every percent child shares the same slot. */
    auto resolvedH = [&](Element* e) -> float {
        Dimension dim = e->getModifier().getHeight();
        return dim.percent ? (dim.value / 100.f * pctSlotH) : dim.value * scale;
    };

    auto slotSizeY = [&](Element* e) -> float {
        Dimension dim = e->getModifier().getHeight();
        return dim.percent ? pctSlotH : dim.value * scale;
    };

    /* Group heights, so the centre and bottom groups know where to start. */
    float midH = 0.f;
    float botH = 0.f;

    for (auto* e : mid) midH += resolvedH(e);
    for (auto* e : bot) botH += resolvedH(e);

    /* Place a group from startY, nudging each child within its own slot
       according to that child's alignment. */
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
            slot.position   = { area.position.x, slotY };
            slot.size       = { area.size.x, sh};
            child->tick(slot, dt);
            cursorY += rh;
        }
    };

    /* Top from the top edge, mid centred, bottom anchored to the bottom edge. */
    layoutGroup(top, area.position.y);
    layoutGroup(mid, area.position.y + (area.size.y - midH) * 0.5f);
    layoutGroup(bot, area.position.y + area.size.y - botH);
}


/*
    layoutResizers(float dt, float scale):
    - Params:   float dt, float scale
    - Returns:  void
    - Desc:     Places the column's Resizer children, which sit at the boundary
                between their two nearest visible neighbours rather than in the
                layout flow, so they take no space from it. The boundary is the
                midpoint of the gap between those neighbours, padding included,
                and falls back to the container edge when a resizer has a
                neighbour on only one side. Horizontally the resizer is sized
                from its Modifier width, clamped to whatever span its neighbours
                have in common and aligned within it, so a handle can be a short
                bar rather than the full width. Its hit height comes from the
                Modifier and is centred on the boundary, and it is given the
                neighbour its direction points at as the element it drags.
*/
void Column::layoutResizers(float dt, float scale) {
    /* Children live inside the inner padding; the container's own bounds do not move. */
    const Rectf area = contentArea();

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
                                + prevEl->getOuterPadding() * scale)
                             : area.position.y;
        float tEdge = nextEl ? (nextEl->getBounds().position.y
                                - nextEl->getOuterPadding() * scale)
                             : (area.position.y + area.size.y);

        const bool hasPrev = (prevEl != nullptr);
        const bool hasNext = (nextEl != nullptr);
        float boundY = hasPrev && hasNext ? (bEdge + tEdge) * 0.5f
                                          : (hasPrev ? bEdge : (hasNext ? tEdge : (area.position.y + area.size.y * 0.5f)));

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
            lftX = area.position.x;
            rgtX = area.position.x + area.size.x;
        }

        Dimension hitDim = r->getModifier().getHeight();
        float hitH = hitDim.percent ? (area.size.y * hitDim.value / 100.f)
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
}


/*
    tickHiddenChildren(float dt):
    - Params:   float dt
    - Returns:  void
    - Desc:     Ticks the children layout skipped, but only on a forced tree
                update. A hidden child takes no space and is never placed, so
                without this it would keep whatever bounds it had when it was
                last visible; giving it a zero-size slot at the column's own
                origin clears them instead.
*/
void Column::tickHiddenChildren(float dt) {
    if (!m_uiloRef || !m_uiloRef->isForcingTreeUpdate()) return;

    /* Children live inside the inner padding; the container's own bounds do not move. */
    const Rectf area = contentArea();


    for (auto* child : m_children) {
        if (child->getType() == ElementType::Resizer) continue;
        if (child->getModifier().isFloating()) continue;
        if (child->getModifier().getVisible()) continue;

        Rectf slot = child->getBounds();
        if (slot.size.x <= 0.f || slot.size.y <= 0.f) {
            slot.position = area.position;
            slot.size = {0.f, 0.f};
        }
        child->tick(slot, dt);
    }
}


/*
    render():
    - Params:   none
    - Returns:  void
    - Desc:     Draws the column: background, then the subdivision grid, then
                its children. The grid goes between the two so it sits behind
                every child. A Material on the Modifier makes the whole subtree
                one glass group, so the children composite into the blur the
                background established rather than each blurring separately.
*/
void Column::render() {
    if (!m_modifier.getVisible()) return;
    if (m_bounds.size.x <= 0.f || m_bounds.size.y <= 0.f) return;

    const float scale = m_uiloRef ? m_uiloRef->getScale() : 1.f;

    renderBackground(scale);
    renderSubdivisions(scale);

    const bool glassSubtree = m_uiloRef
        && m_modifier.getMaterial().kind != Material::Kind::None;

    if (glassSubtree) m_uiloRef->getRenderer().beginGlassSubtree();
    renderChildren();
    if (glassSubtree) m_uiloRef->getRenderer().endGlassSubtree();

    m_dirty = false;
}


/*
    renderBackground(float scale):
    - Params:   float scale
    - Returns:  void
    - Desc:     Draws the column's background. A Material takes precedence and
                owns the background outright -- it draws its own rounded rect,
                tint and effect, so the flat fill is skipped rather than drawn
                underneath it. Otherwise an active gradient wins over the flat
                colour, and either is drawn with whatever outline the options
                ask for. Nothing is drawn at all when the fill is transparent
                and there is no outline.
*/
void Column::renderBackground(float scale) {
    if (!m_uiloRef) return;

    const Color bg = m_uiloRef->getPalette().resolve(
        m_options.getColorRole(), m_options.getColor());

    const Material& mat = m_modifier.getMaterial();
    if (mat.kind != Material::Kind::None) {
        m_uiloRef->getRenderer().drawGlass(m_bounds, mat, bg);
        return;
    }

    float r = m_options.getRounding() * scale;
    const Color ol = m_uiloRef->getPalette().resolve(
        m_options.getOutlineColorRole(), m_options.getOutlineColor());
    const float olT = m_options.getOutlineThickness() * scale;

    Color gc[4];
    if (resolveGradient(m_options.getGradient(),
                        m_options.getGradientRole(), gc)) {
        if (r <= 0.f) {
            Rect shape{m_bounds.position, m_bounds.size, Color::White, ol, olT};
            shape.setGradientColors(gc);
            m_uiloRef->getRenderer().draw(shape);
        } else {
            RoundedRect shape{m_bounds.position, m_bounds.size, r, 8u,
                              Color::White, ol, olT};
            shape.setGradientColors(gc);
            m_uiloRef->getRenderer().draw(shape);
        }
    } else if (bg.a > 0 || olT > 0.f) {
        if (r <= 0.f)
            m_uiloRef->getRenderer().draw(
                Rect{m_bounds.position, m_bounds.size, bg, ol, olT});
        else
            m_uiloRef->getRenderer().draw(
                RoundedRect{m_bounds.position, m_bounds.size, r, 8u, bg, ol, olT});
    }
}


/*
    renderSubdivisions(float scale):
    - Params:   float scale
    - Returns:  void
    - Desc:     Draws the subdivision grid behind the column's children,
                positioned against the scroll offset so it travels with the
                content. Only a scrollable column draws one. The step is
                normalised to a stable on-screen density, so zooming coarsens or
                refines the grid rather than crowding the lines together; minor
                lines subdivide each major step and are drawn fainter, and an
                optional stripe fills alternating bands of major steps. Every
                set is batched into one draw call, and only the part crossing
                the viewport is emitted.
*/
void Column::renderSubdivisions(float scale) {
    if (!m_uiloRef) return;
    if (m_options.getSubDivisions() <= 0.f) return;
    if (!m_options.getScrollable()) return;

    auto& renderer   = m_uiloRef->getRenderer();
    const float zf   = m_options.getZoomableY() ? m_zoomY : 1.f;
    const float base = m_options.getSubDivisions() * scale * zf;
    const float minPx = std::max(1.f, m_options.getSubDivisionMinScreenPx()) * scale;
    const Color divColor = resolveColor(m_options.getSubDivisionColorRole(),
                                        m_options.getSubDivisionColor());
    const unsigned int segmentCount = std::max(1u, m_options.getSubDivisionMajor() + m_options.getSubDivisionMinor());
    if (base <= 0.f || divColor.a == 0 || segmentCount == 0u) return;

    const float viewTop    = m_scrollViewportY;
    const float viewBottom = viewTop + m_scrollViewportHeight;
    const float left       = m_bounds.position.x;
    const float right      = m_bounds.position.x + m_bounds.size.x;
    auto positiveMod = [](float value, float period) {
        float mod = std::fmod(value, period);
        if (mod < 0.f) mod += period;
        return mod;
    };

    /* Normalise the major step to a stable on-screen density, so zooming
       coarsens or refines the grid instead of crowding the lines. */
    const float segmentRatio = static_cast<float>(segmentCount);
    const float minDistanceBase = m_options.getSubDivisionsMinDistance() > 0.f
        ? m_options.getSubDivisionsMinDistance()
        : m_options.getSubDivisionMinScreenPx();
    const float maxDistanceBase = m_options.getSubDivisionsMaxDistance() > 0.f
        ? m_options.getSubDivisionsMaxDistance()
        : m_options.getSubDivisions();
    const float minDistance = std::max(minPx, minDistanceBase * scale);
    const float maxDistance = std::max(minDistance, maxDistanceBase * scale);

    const float majorStep = normalizeGridStep(base,
                                              segmentRatio,
                                              minDistance,
                                              maxDistance);
    const float minorStep = segmentCount > 1u ? (majorStep / segmentRatio) : 0.f;

    /* Stripe bands: every other run of stripeEvery major steps is filled, so
       the pair period is twice the stripe width. */
    const unsigned int stripeEvery = std::max(1u, m_options.getSubDivisionStripeEvery());
    const Color stripeColor = resolveColor(m_options.getSubDivisionStripeColorRole(),
                                           m_options.getSubDivisionStripeColor());
    if (m_options.getSubDivisionStripeEvery() > 0u && stripeColor.a > 0 && majorStep > 0.f) {
        const float stripeStep = majorStep * static_cast<float>(stripeEvery);
        if (stripeStep > 0.f) {
            const float pairStep = stripeStep * 2.f;
            const float pairOffset = positiveMod(m_scrollOffset, pairStep);
            const float firstBandY = viewTop - pairOffset;
            for (float bandY = firstBandY; bandY <= viewBottom + 0.5f; bandY += pairStep) {
                const float drawY0 = std::max(viewTop, bandY);
                const float drawY1 = std::min(viewBottom, bandY + stripeStep);
                if (drawY1 <= drawY0) continue;
                renderer.draw(Rect{{left, drawY0}, {right - left, drawY1 - drawY0}, stripeColor});
            }
        }
    }

    if (segmentCount > 1u && minorStep > 0.f) {
        Color minorColor = divColor;
        minorColor.a = static_cast<uint8_t>(static_cast<float>(divColor.a) * 0.45f);
        const float minorOffset = positiveMod(m_scrollOffset, minorStep);
        const float firstMinorY = viewTop - minorOffset;
        std::vector<Line> minorLines;
        const size_t minorEstimate = static_cast<size_t>(
            std::max(0.f, (viewBottom + 0.5f - firstMinorY) / minorStep) + 1.f);
        minorLines.reserve(minorEstimate);
        for (float y = firstMinorY; y <= viewBottom + 0.5f; y += minorStep)
            minorLines.push_back(Line{{left, y}, {right, y}, 1.f, minorColor});
        if (!minorLines.empty())
            renderer.drawLines(minorLines.data(), minorLines.size());
    }

    if (majorStep > 0.f) {
        const float majorOffset = positiveMod(m_scrollOffset, majorStep);
        const float firstMajorY = viewTop - majorOffset;
        std::vector<Line> majorLines;
        const size_t majorEstimate = static_cast<size_t>(
            std::max(0.f, (viewBottom + 0.5f - firstMajorY) / majorStep) + 1.f);
        majorLines.reserve(majorEstimate);
        for (float y = firstMajorY; y <= viewBottom + 0.5f; y += majorStep)
            majorLines.push_back(Line{{left, y}, {right, y}, 1.f, divColor});
        if (!majorLines.empty())
            renderer.drawLines(majorLines.data(), majorLines.size());
    }
}


/*
    renderChildren():
    - Params:   none
    - Returns:  void
    - Desc:     Draws the children in two passes, scrolling content first and
                pinned content over the top, so a pinned child is never buried
                by content sliding underneath it.
*/
void Column::renderChildren() {
    /* True when pinned children reserved space, so the scrolling area is
       shorter than the column itself. */
    const bool viewportInset = m_options.getScrollable()
        && m_scrollViewportHeight > 0.f
        && (m_scrollViewportY > m_bounds.position.y
            || m_scrollViewportHeight < m_bounds.size.y);

    renderChildPass(false, viewportInset);
    renderChildPass(true, viewportInset);

    /* Above everything else the container holds. */
    renderFloating(m_options.getRounding());
}


/*
    renderChildPass(bool ignoreScrollChildren, bool viewportInset):
    - Params:   bool ignoreScrollChildren, bool viewportInset
    - Returns:  void
    - Desc:     Draws one pass of children, either the scrolling ones or the
                pinned ones. Each is clipped to the column so content cannot
                spill past a rounded corner, and scrolling content is clipped
                again to the scroll viewport so it cannot bleed into the strip a
                pinned child owns and be sliced by that child's background.
*/
void Column::renderChildPass(bool ignoreScrollChildren, bool viewportInset) {
    for (auto* child : m_children) {
        if (child->getType() == ElementType::Resizer) continue;
        if (child->getModifier().isFloating()) continue;
        if (child->getModifier().getIgnoreScroll() != ignoreScrollChildren) continue;

        bool clippedToViewport = false;
        if (m_uiloRef) {
            const float rr = m_options.getRounding() * (m_uiloRef->getScale());
            m_uiloRef->getRenderer().pushRoundClip(m_bounds, rr);

            if (!ignoreScrollChildren && viewportInset) {
                Rectf viewport = m_bounds;
                viewport.position.y = m_scrollViewportY;
                viewport.size.y     = m_scrollViewportHeight;
                m_uiloRef->getRenderer().pushRoundClip(viewport, 0.f);
                clippedToViewport = true;
            }
        }

        child->render();

        if (m_uiloRef) {
            if (clippedToViewport) m_uiloRef->getRenderer().popRoundClip();
            m_uiloRef->getRenderer().popRoundClip();
        }
    }
}


/*
    checkZoom(const Vec2f& mousePosition, float magnification):
    - Params:   const Vec2f& mousePosition, float magnification
    - Returns:  bool -- true when the column consumed the gesture
    - Desc:     Applies a pinch or scroll-zoom to the column's vertical zoom,
                offering it to the children first so a nested zoomable element
                wins. The scroll offset is then rewritten so the content under
                the cursor stays under the cursor: the content coordinate at the
                pointer is read at the old zoom, then converted back into an
                offset at the new one.
*/
bool Column::checkZoom(const Vec2f& mousePosition, float magnification) {
    if (!m_bounds.contains(mousePosition)) return false;

    if (Container::checkZoom(mousePosition, magnification)) return true;

    if (!m_options.getZoomableY()) return false;

    const float oldZoom = m_zoomY;
    m_zoomY = std::clamp(m_zoomY * (1.f + magnification),
                         m_options.getZoomMin(), m_options.getZoomMax());
    if (m_zoomY == oldZoom) return false;

    const float sc   = m_uiloRef ? m_uiloRef->getScale() : 1.f;
    const float vt   = m_scrollViewportY;
    const float mRel = mousePosition.y - vt;
    const float content = (mRel + m_scrollOffset) / (sc * oldZoom);

    applyScrollOffset(content * sc * m_zoomY - mRel);
    return true;
}


/*
    setZoomY(float z):
    - Params:   float z
    - Returns:  void
    - Desc:     Sets the vertical zoom directly, clamped to the configured
                range. Unlike checkZoom this does not preserve the point under
                the cursor, since there is no gesture to anchor to.
*/
void Column::setZoomY(float z) {
    m_zoomY = std::clamp(z, m_options.getZoomMin(), m_options.getZoomMax());
    m_dirty = true;
}


/*
    checkScroll(const Vec2f& mousePosition, float delta, bool precise, bool momentum):
    - Params:   const Vec2f& mousePosition, float delta, bool precise, bool
                momentum
    - Returns:  bool -- true when the column consumed the event
    - Desc:     Single-axis scroll, offered to the children first. `precise`
                marks a trackpad's pixel delta, which is scaled against
                scrollSpeed differently from a wheel's discrete step because the
                OS already supplies the momentum tail.
    - A floating child under the pointer takes the event and the column does not
      scroll, so the view does not slide out from under a panel laid over it.
*/
bool Column::checkScroll(
    const Vec2f& mousePosition,
    float delta,
    bool precise,
    bool momentum
) {
    if (!m_bounds.contains(mousePosition)) return false;

    if (Element* top = floatingAt(mousePosition)) {
        top->checkScroll(mousePosition, delta, precise, momentum);
        return true;
    }

    if (m_options.getScrollable()) {
        for (auto* child : m_children)
            if (child->getBounds().contains(mousePosition))
                if (child->checkScroll(mousePosition, delta, precise, momentum)) return true;

        if (!canUseLooseScrollBounds(m_options, contentOverflow())) return false;

        applyScrollOffset(m_scrollOffset - delta * scrollStep(precise));
        return true;
    }

    return Container::checkScroll(mousePosition, delta, precise, momentum);
}


/*
    checkScroll(const Vec2f& mousePosition, Vec2f delta, bool precise, bool momentum):
    - Params:   const Vec2f& mousePosition, Vec2f delta, bool precise, bool
                momentum
    - Returns:  bool -- true when the column consumed the event
    - Desc:     Two-axis scroll. The children are offered the full delta first,
                but their answer is deliberately not short-circuited: a column
                owns delta.y and applies it whatever a child Row does with
                delta.x, so a nested pair scrolls on both axes from one gesture.
                Falls back to the Modifier's onScroll handler when nothing
                consumed the event.
*/
bool Column::checkScroll(
    const Vec2f& mousePosition,
    Vec2f delta,
    bool precise,
    bool momentum
) {
    if (!m_bounds.contains(mousePosition)) return false;

    if (Element* top = floatingAt(mousePosition)) {
        top->checkScroll(mousePosition, delta, precise, momentum);
        return true;
    }

    bool consumed = false;
    for (auto* child : m_children)
        if (child->getBounds().contains(mousePosition))
            if (child->checkScroll(mousePosition, delta, precise, momentum)) { consumed = true; break; }

    if (m_options.getScrollable() && delta.y != 0.f) {
        if (canUseLooseScrollBounds(m_options, contentOverflow())) {
            applyScrollOffset(m_scrollOffset - delta.y * scrollStep(precise));
            consumed = true;
        }
    }

    if (!consumed && m_modifier.getOnScroll()) {
        m_modifier.getOnScroll()(this, delta.y);
        return true;
    }

    return consumed;
}

}
