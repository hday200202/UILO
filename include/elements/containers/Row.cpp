#include "Row.hpp"
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
    - Params:   const RowOptions& options, float contentMax, float& minScroll,
                float& maxScroll
    - Returns:  void
    - Desc:     Works out how far a row may scroll. With both explicit bounds
                set the options win outright, swapped into order if they arrive
                reversed; otherwise the range runs from 0 to however much
                content overflows the viewport.
*/
void resolveScrollBounds(
    const RowOptions& options,
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
    canUseLooseScrollBounds(const RowOptions& options, float contentMax):
    - Params:   const RowOptions& options, float contentMax
    - Returns:  bool
    - Desc:     Whether a scroll event should be consumed at all. Explicit
                bounds always allow it; otherwise there has to be overflow to
                scroll into, so a row whose content fits lets the event bubble
                to its parent.
*/
bool canUseLooseScrollBounds(const RowOptions& options, float contentMax) {
    return options.hasScrollMin() && options.hasScrollMax()
        ? true
        : contentMax > 0.f;
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
    Row(...):
    - Params:   Modifier modifier, RowOptions options, contains children,
                const std::string& name
    - Returns:  Row
    - Desc:     Constructs a row from a modifier, its options and its children,
                and tags it as a Row so layout and hit-testing can identify it.
*/
Row::Row(
    Modifier modifier,
    RowOptions options,
    contains children,
    const std::string& name
) : Container(modifier, children, name), m_options(options) {
    m_type = ElementType::Row;
}


/*
    contentOverflow():
    - Params:   none
    - Returns:  float
    - Desc:     How much content sticks out past the scrolling viewport, never
                negative. This is the distance the row can travel when no
                explicit scroll bounds were set.
*/
float Row::contentOverflow() const {
    return std::max(0.f, m_contentWidth - m_scrollViewportWidth);
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
float Row::scrollStep(bool precise) const {
    const float speed = m_options.getScrollSpeed();
    return precise ? 30.f * (speed / 40.f) : speed;
}


/*
    readScrollLinks():
    - Params:   none
    - Returns:  void
    - Desc:     Takes the shared scroll offset and zoom from the link groups
                this row belongs to. Linked rows travel together, so the shared
                value has to be adopted before layout rather than after it.
*/
void Row::readScrollLinks() {
    if (!m_uiloRef) return;

    if (!m_options.getScrollLink().empty())
        m_scrollOffset = m_uiloRef->getScrollLinkOffset(m_options.getScrollLink(), true);

    if (!m_options.getZoomLink().empty())
        m_zoomX = m_uiloRef->getZoomLinkValue(m_options.getZoomLink(), true);
}


/*
    publishScrollLinks():
    - Params:   none
    - Returns:  void
    - Desc:     Writes this row's scroll offset and zoom back to its link
                groups, so every other row sharing the same id picks them up on
                its next update.
*/
void Row::publishScrollLinks() const {
    if (!m_uiloRef) return;

    if (!m_options.getScrollLink().empty())
        m_uiloRef->setScrollLinkOffset(m_options.getScrollLink(), m_scrollOffset, true);

    if (!m_options.getZoomLink().empty())
        m_uiloRef->setZoomLinkValue(m_options.getZoomLink(), m_zoomX, true);
}


/*
    applyScrollOffset(float target):
    - Params:   float target
    - Returns:  void
    - Desc:     Moves the row to an absolute offset, clamped into the range the
                current content allows, then publishes it to any linked row and
                marks the row dirty.
*/
void Row::applyScrollOffset(float target) {
    float minScroll = 0.f;
    float maxScroll = 0.f;
    resolveScrollBounds(m_options, contentOverflow(), minScroll, maxScroll);

    m_scrollOffset = std::clamp(target, minScroll, maxScroll);
    publishScrollLinks();
    m_dirty = true;
}


/*
    update(Rectf& parentBounds, float dt):
    - Params:   Rectf& parentBounds, float dt
    - Returns:  void
    - Desc:     Lays the row's children out along the horizontal axis and ticks
                each one with the slot it was given. Resolves the row's own
                bounds, adopts any linked scroll offset and zoom, then hands the
                children to whichever of the two layout paths applies:
                updateScrollable when the row scrolls, updateFlow when it does
                not. Resizers and hidden children are dealt with afterwards in
                both cases, since neither takes part in the flow.
*/
void Row::update(Rectf& parentBounds, float dt) {
    pruneChildren();
    resize(parentBounds);

    /* Keep the scroll offset proportional when the UI scale changes. */
    const float scale = m_uiloRef ? m_uiloRef->getScale() : 1.f;
    if (scale != m_lastScale && m_lastScale > 0.f) {
        m_scrollOffset *= scale / m_lastScale;
        m_lastScale = scale;
    }

    readScrollLinks();

    m_scrollViewportWidth = m_bounds.size.x;

    if (m_options.getScrollable()) updateScrollable(dt, scale);
    else                           updateFlow(dt, scale);

    layoutResizers(dt, scale);
    tickHiddenChildren(dt);
}


/*
    updateScrollable(float dt, float scale):
    - Params:   float dt, float scale
    - Returns:  void
    - Desc:     Lays out a scrolling row. Pinned children -- those with
                Modifier::ignoreScroll -- are placed first, grouped left, centre
                and right, and the width they take is reserved: whatever is left
                between the left and right strips becomes the scrolling
                viewport, so a pinned header stays put while the rest of the row
                travels. The scrolling children are then placed from the
                viewport's left edge displaced by the scroll offset, their total
                measured as the content width, and the offset finally clamped
                into the range that content allows and published to any linked
                row.
*/
void Row::updateScrollable(float dt, float scale) {
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

    /* Sort the pinned children into left, centre and right groups. */
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

    /* Whatever the pinned strips did not take is the scrolling viewport. */
    Rectf scrollViewport = m_bounds;
    scrollViewport.position.x += pinnedLeftW;
    scrollViewport.size.x = std::max(0.f, m_bounds.size.x - pinnedLeftW - pinnedRightW);
    m_scrollViewportWidth = scrollViewport.size.x;

    /* Lay the scrolling children out, displaced by the scroll offset. */
    float cursorX = scrollViewport.position.x - m_scrollOffset;
    m_contentWidth = 0.f;
    for (auto* child : m_children) {
        if (!child->getModifier().getVisible()) continue;
        if (child->getType() == ElementType::Resizer) continue;
        if (child->getModifier().getIgnoreScroll()) continue;

        Dimension dim = child->getModifier().getWidth();
        const float zf = m_options.getZoomableX() ? m_zoomX : 1.f;
        float rw = dim.percent ? (scrollViewport.size.x * dim.value / 100.f) : dim.value * scale * zf;
        Rectf slot{ {cursorX, scrollViewport.position.y}, {rw, scrollViewport.size.y} };
        child->tick(slot, dt);
        cursorX       += rw;
        m_contentWidth += rw;
    }

    /* Clamp into the resolved bounds and publish to any linked row. */
    m_scrollViewportX = scrollViewport.position.x;
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
    - Desc:     Lays out a non-scrolling row. Children are sorted into left,
                centre and right buckets by their alignment, then the width left
                over after the fixed-size ones is shared among the percent-sized
                ones -- every percent child gets the same slot, so two 50%
                siblings beside a 100px one split what remains rather than the
                whole row. Each group is finally placed from its own anchor:
                left from the left edge, centre about the middle, right against
                the right edge.
*/
void Row::updateFlow(float dt, float scale) {
    /* First pass: bucket the children by alignment, and total how much width is
       fixed against how many percent units are asking for the rest. */
    std::vector<Element*> left;
    std::vector<Element*> mid;
    std::vector<Element*> right;

    float totalFixed = 0.f;
    float totalPct = 0.f;

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

    /* What is left after the fixed-width children, and the width of a single
       percent-unit slot. */
    const float remaining   = m_bounds.size.x - totalFixed;
    const float pctSlotW    = totalPct > 0.f ? (remaining * 100.f / totalPct) : remaining;

    /* resolvedW is what a child draws at; slotSizeX is what advances the cursor,
       and every percent child shares the same slot. */
    auto resolvedW = [&](Element* e) -> float {
        Dimension dim = e->getModifier().getWidth();
        return dim.percent ? (dim.value / 100.f * pctSlotW) : dim.value * scale;
    };

    auto slotSizeX = [&](Element* e) -> float {
        Dimension dim = e->getModifier().getWidth();
        return dim.percent ? pctSlotW : dim.value * scale;
    };

    /* Group widths, so the centre and right groups know where to start. */
    float midW = 0.f;
    float rightW = 0.f;

    for (auto* e : mid)   midW   += resolvedW(e);
    for (auto* e : right) rightW += resolvedW(e);

    /* Place a group from startX, nudging each child within its own slot
       according to that child's alignment. */
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

    /* Left from the left edge, mid centred, right anchored to the right edge. */
    layoutGroup(left,  m_bounds.position.x);
    layoutGroup(mid,   m_bounds.position.x + (m_bounds.size.x - midW) * 0.5f);
    layoutGroup(right, m_bounds.position.x + m_bounds.size.x - rightW);
}


/*
    layoutResizers(float dt, float scale):
    - Params:   float dt, float scale
    - Returns:  void
    - Desc:     Places the row's Resizer children, which sit at the boundary
                between their two nearest visible neighbours rather than in the
                layout flow, so they take no space from it. The boundary is the
                midpoint of the gap between those neighbours, padding included,
                and falls back to the container edge when a resizer has a
                neighbour on only one side. Vertically the resizer spans
                whatever its neighbours have in common. Its hit width comes from
                the Modifier like any other element, centred on the boundary,
                and it is given the neighbour its direction points at as the
                element it drags.
*/
void Row::layoutResizers(float dt, float scale) {
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
}


/*
    tickHiddenChildren(float dt):
    - Params:   float dt
    - Returns:  void
    - Desc:     Ticks the children layout skipped, but only on a forced tree
                update. A hidden child takes no space and is never placed, so
                without this it would keep whatever bounds it had when it was
                last visible; giving it a zero-size slot at the row's own origin
                clears them instead.
*/
void Row::tickHiddenChildren(float dt) {
    if (!m_uiloRef || !m_uiloRef->isForcingTreeUpdate()) return;

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


/*
    render():
    - Params:   none
    - Returns:  void
    - Desc:     Draws the row: background, then the subdivision grid, then its
                children. The grid goes between the two so it sits behind every
                child. A Material on the Modifier makes the whole subtree one
                glass group, so the children composite into the blur the
                background established rather than each blurring separately.
*/
void Row::render() {
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
    - Desc:     Draws the row's background. A Material takes precedence and owns
                the background outright -- it draws its own rounded rect, tint
                and effect, so the flat fill is skipped rather than drawn
                underneath it. Otherwise an active gradient wins over the flat
                colour, and either is drawn with whatever outline the options
                ask for. Nothing is drawn at all when the fill is transparent
                and there is no outline.
*/
void Row::renderBackground(float scale) {
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
    - Desc:     Draws the subdivision grid behind the row's children, positioned
                against the scroll offset so it travels with the content. Only a
                scrollable row draws one. The step is normalised to a stable on-
                screen density, so zooming coarsens or refines the grid rather
                than crowding the lines together; minor lines subdivide each
                major step and are drawn fainter, and an optional stripe fills
                alternating bands of major steps. Every set is batched into one
                draw call, and only the part crossing the viewport is emitted.
*/
void Row::renderSubdivisions(float scale) {
    if (!m_uiloRef) return;
    if (m_options.getSubDivisions() <= 0.f) return;
    if (!m_options.getScrollable()) return;

    auto& renderer   = m_uiloRef->getRenderer();
    const float zf   = m_options.getZoomableX() ? m_zoomX : 1.f;
    const float base = m_options.getSubDivisions() * scale * zf;
    const float minPx = std::max(1.f, m_options.getSubDivisionMinScreenPx()) * scale;
    const Color divColor = resolveColor(m_options.getSubDivisionColorRole(),
                                        m_options.getSubDivisionColor());
    const unsigned int segmentCount = std::max(1u, m_options.getSubDivisionMajor() + m_options.getSubDivisionMinor());
    if (base <= 0.f || divColor.a == 0 || segmentCount == 0u) return;

    const float viewLeft  = m_scrollViewportX;
    const float viewRight = viewLeft + m_scrollViewportWidth;
    const float top       = m_bounds.position.y;
    const float bot       = m_bounds.position.y + m_bounds.size.y;
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
            const float firstBandX = viewLeft - pairOffset;
            for (float bandX = firstBandX; bandX <= viewRight + 0.5f; bandX += pairStep) {
                const float drawX0 = std::max(viewLeft, bandX);
                const float drawX1 = std::min(viewRight, bandX + stripeStep);
                if (drawX1 <= drawX0) continue;
                renderer.draw(Rect{{drawX0, top}, {drawX1 - drawX0, bot - top}, stripeColor});
            }
        }
    }

    if (segmentCount > 1u && minorStep > 0.f) {
        Color minorColor = divColor;
        minorColor.a = static_cast<uint8_t>(static_cast<float>(divColor.a) * 0.45f);
        const float minorOffset = positiveMod(m_scrollOffset, minorStep);
        const float firstMinorX = viewLeft - minorOffset;
        std::vector<Line> minorLines;
        const size_t minorEstimate = static_cast<size_t>(
            std::max(0.f, (viewRight + 0.5f - firstMinorX) / minorStep) + 1.f);
        minorLines.reserve(minorEstimate);
        for (float x = firstMinorX; x <= viewRight + 0.5f; x += minorStep)
            minorLines.push_back(Line{{x, top}, {x, bot}, 1.f, minorColor});
        if (!minorLines.empty())
            renderer.drawLines(minorLines.data(), minorLines.size());
    }

    if (majorStep > 0.f) {
        const float majorOffset = positiveMod(m_scrollOffset, majorStep);
        const float firstMajorX = viewLeft - majorOffset;
        std::vector<Line> majorLines;
        const size_t majorEstimate = static_cast<size_t>(
            std::max(0.f, (viewRight + 0.5f - firstMajorX) / majorStep) + 1.f);
        majorLines.reserve(majorEstimate);
        for (float x = firstMajorX; x <= viewRight + 0.5f; x += majorStep)
            majorLines.push_back(Line{{x, top}, {x, bot}, 1.f, divColor});
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
void Row::renderChildren() {
    /* True when pinned children reserved space, so the scrolling area is
       narrower than the row itself. */
    const bool viewportInset = m_options.getScrollable()
        && m_scrollViewportWidth > 0.f
        && (m_scrollViewportX > m_bounds.position.x
            || m_scrollViewportWidth < m_bounds.size.x);

    renderChildPass(false, viewportInset);
    renderChildPass(true, viewportInset);
}


/*
    renderChildPass(bool ignoreScrollChildren, bool viewportInset):
    - Params:   bool ignoreScrollChildren, bool viewportInset
    - Returns:  void
    - Desc:     Draws one pass of children, either the scrolling ones or the
                pinned ones. Each is clipped to the row so content cannot spill
                past a rounded corner, and scrolling content is clipped again to
                the scroll viewport so it cannot bleed into the strip a pinned
                child owns and be sliced by that child's background.
*/
void Row::renderChildPass(bool ignoreScrollChildren, bool viewportInset) {
    for (auto* child : m_children) {
        if (child->getType() == ElementType::Resizer) continue;
        if (child->getModifier().getIgnoreScroll() != ignoreScrollChildren) continue;

        bool clippedToViewport = false;
        if (m_uiloRef) {
            const float rr = m_options.getRounding() * (m_uiloRef->getScale());
            m_uiloRef->getRenderer().pushRoundClip(m_bounds, rr);

            if (!ignoreScrollChildren && viewportInset) {
                Rectf viewport = m_bounds;
                viewport.position.x = m_scrollViewportX;
                viewport.size.x     = m_scrollViewportWidth;
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
    - Returns:  bool -- true when the row consumed the gesture
    - Desc:     Applies a pinch or scroll-zoom to the row's horizontal zoom,
                offering it to the children first so a nested zoomable element
                wins. The scroll offset is then rewritten so the content under
                the cursor stays under the cursor: the content coordinate at the
                pointer is read at the old zoom, then converted back into an
                offset at the new one.
*/
bool Row::checkZoom(const Vec2f& mousePosition, float magnification) {
    if (!m_bounds.contains(mousePosition)) return false;

    if (Container::checkZoom(mousePosition, magnification)) return true;

    if (!m_options.getZoomableX()) return false;

    const float oldZoom = m_zoomX;
    m_zoomX = std::clamp(m_zoomX * (1.f + magnification),
                         m_options.getZoomMin(), m_options.getZoomMax());
    if (m_zoomX == oldZoom) return false;

    const float sc   = m_uiloRef ? m_uiloRef->getScale() : 1.f;
    const float vl   = m_scrollViewportX;
    const float mRel = mousePosition.x - vl;
    const float content = (mRel + m_scrollOffset) / (sc * oldZoom);

    applyScrollOffset(content * sc * m_zoomX - mRel);
    return true;
}


/*
    setZoomX(float z):
    - Params:   float z
    - Returns:  void
    - Desc:     Sets the horizontal zoom directly, clamped to the configured
                range. Unlike checkZoom this does not preserve the point under
                the cursor, since there is no gesture to anchor to.
*/
void Row::setZoomX(float z) {
    m_zoomX = std::clamp(z, m_options.getZoomMin(), m_options.getZoomMax());
    m_dirty = true;
}


/*
    checkScroll(const Vec2f& mousePosition, float delta, bool precise, bool momentum):
    - Params:   const Vec2f& mousePosition, float delta, bool precise,
                bool momentum
    - Returns:  bool -- true when the row consumed the event
    - Desc:     Single-axis scroll, offered to the children first. A row scrolls
                horizontally, so it declines a zero delta and lets plain
                vertical wheel events bubble to a parent that can use them.
                `precise` marks a trackpad's pixel delta, which is scaled
                differently from a wheel's discrete step.
*/
bool Row::checkScroll(
    const Vec2f& mousePosition,
    float delta,
    bool precise,
    bool momentum
) {
    if (!m_bounds.contains(mousePosition)) return false;

    if (m_options.getScrollable()) {
        for (auto* child : m_children)
            if (child->getBounds().contains(mousePosition))
                if (child->checkScroll(mousePosition, delta, precise, momentum)) return true;

        if (delta == 0.f) return false;
        if (!canUseLooseScrollBounds(m_options, contentOverflow())) return false;

        applyScrollOffset(m_scrollOffset - delta * scrollStep(precise));
        return true;
    }

    return Container::checkScroll(mousePosition, delta, precise, momentum);
}


/*
    checkScroll(const Vec2f& mousePosition, Vec2f delta, bool precise, bool momentum):
    - Params:   const Vec2f& mousePosition, Vec2f delta, bool precise,
                bool momentum
    - Returns:  bool -- true when the row consumed the event
    - Desc:     Two-axis scroll. The children are offered the full delta first,
                but their answer is deliberately not short-circuited: a row owns
                delta.x and applies it whatever a parent Column does with
                delta.y, so a nested pair scrolls on both axes from one gesture.
                Falls back to the Modifier's onScroll handler when nothing
                consumed the event.
*/
bool Row::checkScroll(
    const Vec2f& mousePosition,
    Vec2f delta,
    bool precise,
    bool momentum
) {
    if (!m_bounds.contains(mousePosition)) return false;

    bool consumed = false;
    for (auto* child : m_children)
        if (child->getBounds().contains(mousePosition))
            if (child->checkScroll(mousePosition, delta, precise, momentum)) { consumed = true; break; }

    if (m_options.getScrollable() && delta.x != 0.f) {
        if (canUseLooseScrollBounds(m_options, contentOverflow())) {
            applyScrollOffset(m_scrollOffset - delta.x * scrollStep(precise));
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
