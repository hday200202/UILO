#include "Row.hpp"
#include "../../UILO.hpp"
#include "../../utils/RenderUtils.hpp"
#include "../interactible/Resizer.hpp"
#include <algorithm>
#include <cmath>
#include <vector>

namespace uilo {

namespace {
void resolveScrollBounds(const RowOptions& options, float contentMax, float& minScroll, float& maxScroll) {
    if (options.hasScrollMin() && options.hasScrollMax()) {
        minScroll = options.getScrollMin();
        maxScroll = options.getScrollMax();
        if (minScroll > maxScroll) std::swap(minScroll, maxScroll);
        return;
    }

    minScroll = 0.f;
    maxScroll = std::max(0.f, contentMax);
}

bool canUseLooseScrollBounds(const RowOptions& options, float contentMax) {
    return options.hasScrollMin() && options.hasScrollMax()
        ? true
        : contentMax > 0.f;
}

float normalizeGridStep(float step,
                        float ratio,
                        float minDistance,
                        float maxDistance) {
    if (step <= 0.f || minDistance <= 0.f || maxDistance <= 0.f) return step;
    if (ratio <= 1.f) ratio = 2.f;
    if (maxDistance < minDistance) maxDistance = minDistance;

    // Keep a stable major-step envelope across zoom by repeatedly coarsening
    // or refining until step lands inside the configured thresholds.
    while (step < minDistance) step *= ratio;
    while (step > maxDistance) step /= ratio;
    return step;
}
}

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

    if (m_uiloRef && !m_options.getScrollLink().empty()) {
        m_scrollOffset = m_uiloRef->getScrollLinkOffset(m_options.getScrollLink(), true);
    }

    if (m_uiloRef && !m_options.getZoomLink().empty()) {
        m_zoomX = m_uiloRef->getZoomLinkValue(m_options.getZoomLink(), true);
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
            const float zf = m_options.getZoomableX() ? m_zoomX : 1.f;
            float rw = dim.percent ? (scrollViewport.size.x * dim.value / 100.f) : dim.value * scale * zf;
            Rectf slot{ {cursorX, scrollViewport.position.y}, {rw, scrollViewport.size.y} };
            child->tick(slot, dt);
            cursorX       += rw;
            m_contentWidth += rw;
        }
        m_scrollViewportX = scrollViewport.position.x;
        const float contentMax = std::max(0.f, m_contentWidth - scrollViewport.size.x);
        float minScroll = 0.f;
        float maxScroll = 0.f;
        resolveScrollBounds(m_options, contentMax, minScroll, maxScroll);
        const float clamped     = std::clamp(m_scrollOffset, minScroll, maxScroll);
        if (clamped != m_scrollOffset) { m_scrollOffset = clamped; m_dirty = true; }
        if (m_uiloRef && !m_options.getScrollLink().empty())
            m_uiloRef->setScrollLinkOffset(m_options.getScrollLink(), m_scrollOffset, true);
        if (m_uiloRef && !m_options.getZoomLink().empty())
            m_uiloRef->setZoomLinkValue(m_options.getZoomLink(), m_zoomX, true);

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

    // Subdivision grid (drawn after background, before children) -------
    if (m_options.getSubDivisions() > 0.f && m_options.getScrollable() && m_uiloRef) {
        auto& renderer   = m_uiloRef->getRenderer();
        const float zf   = m_options.getZoomableX() ? m_zoomX : 1.f;
        const float base = m_options.getSubDivisions() * scale * zf;
        const float minPx = std::max(1.f, m_options.getSubDivisionMinScreenPx());
        const Color divColor = resolveColor(m_options.getSubDivisionColorRole(),
                                            m_options.getSubDivisionColor());
        const unsigned int segmentCount = std::max(1u, m_options.getSubDivisionMajor() + m_options.getSubDivisionMinor());
        if (base > 0.f && divColor.a > 0 && segmentCount > 0u) {
            const float viewLeft  = m_scrollViewportX;
            const float viewRight = viewLeft + m_scrollViewportWidth;
            const float top       = m_bounds.position.y;
            const float bot       = m_bounds.position.y + m_bounds.size.y;
            auto positiveMod = [](float value, float period) {
                float mod = std::fmod(value, period);
                if (mod < 0.f) mod += period;
                return mod;
            };

            const float segmentRatio = static_cast<float>(segmentCount);
            const float minDistance = std::max(
                minPx,
                m_options.getSubDivisionsMinDistance() > 0.f
                    ? m_options.getSubDivisionsMinDistance() * scale
                    : minPx
            );
            const float maxDistance = std::max(
                minDistance,
                m_options.getSubDivisionsMaxDistance() > 0.f
                    ? m_options.getSubDivisionsMaxDistance() * scale
                    : (m_options.getSubDivisions() * scale)
            );

            const float majorStep = normalizeGridStep(base,
                                                      segmentRatio,
                                                      minDistance,
                                                      maxDistance);
            const float minorStep = segmentCount > 1u ? (majorStep / segmentRatio) : 0.f;

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
    }

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

bool Row::checkZoom(const Vec2f& mousePosition, float magnification) {
    if (!m_bounds.contains(mousePosition)) return false;

    if (Container::checkZoom(mousePosition, magnification)) return true;

    if (!m_options.getZoomableX()) return false;

    const float oldZoom = m_zoomX;
    m_zoomX = std::clamp(m_zoomX * (1.f + magnification),
                         m_options.getZoomMin(), m_options.getZoomMax());
    if (m_zoomX == oldZoom) return false;

    // Keep the content point under the cursor stationary.
    // contentAtMouse (in base-scale px) = (mouseX - viewportLeft + scrollOffset) / (scale * oldZoom)
    // newScrollOffset = contentAtMouse * scale * newZoom - (mouseX - viewportLeft)
    const float sc   = m_uiloRef ? m_uiloRef->getScale() : 1.f;
    const float vl   = m_scrollViewportX;
    const float mRel = mousePosition.x - vl;
    const float content = (mRel + m_scrollOffset) / (sc * oldZoom);
    float minScroll = 0.f;
    float maxScroll = 0.f;
    resolveScrollBounds(m_options, std::max(0.f, m_contentWidth - m_scrollViewportWidth), minScroll, maxScroll);
    m_scrollOffset = std::clamp(content * sc * m_zoomX - mRel, minScroll, maxScroll);
    if (m_uiloRef && !m_options.getScrollLink().empty())
        m_uiloRef->setScrollLinkOffset(m_options.getScrollLink(), m_scrollOffset, true);
    if (m_uiloRef && !m_options.getZoomLink().empty())
        m_uiloRef->setZoomLinkValue(m_options.getZoomLink(), m_zoomX, true);

    m_dirty = true;
    return true;
}

void Row::setZoomX(float z) {
    m_zoomX = std::clamp(z, m_options.getZoomMin(), m_options.getZoomMax());
    m_dirty = true;
}

bool Row::checkScroll(const Vec2f& mousePosition, float delta, bool precise, bool momentum) {
    if (!m_bounds.contains(mousePosition)) return false;

    if (m_options.getScrollable()) {
        for (auto* child : m_children)
            if (child->getBounds().contains(mousePosition))
                if (child->checkScroll(mousePosition, delta, precise, momentum)) return true;

        // Rows are horizontally scrollable. Let plain vertical wheel events
        // bubble up to parent containers so the timeline itself can scroll.
        if (delta == 0.f) return false;

        const float speed     = m_options.getScrollSpeed();
        const float step      = precise ? 30.f * (speed / 40.f) : speed;
        const float contentMax = std::max(0.f, m_contentWidth - m_scrollViewportWidth);
        if (!canUseLooseScrollBounds(m_options, contentMax)) return false;
        float minScroll = 0.f;
        float maxScroll = 0.f;
        resolveScrollBounds(m_options, contentMax, minScroll, maxScroll);
        m_scrollOffset = std::clamp(m_scrollOffset - delta * step, minScroll, maxScroll);
        if (m_uiloRef && !m_options.getScrollLink().empty())
            m_uiloRef->setScrollLinkOffset(m_options.getScrollLink(), m_scrollOffset, true);
        if (m_uiloRef && !m_options.getZoomLink().empty())
            m_uiloRef->setZoomLinkValue(m_options.getZoomLink(), m_zoomX, true);
        m_dirty = true;
        return true;
    }

    return Container::checkScroll(mousePosition, delta, precise, momentum);
}

bool Row::checkScroll(const Vec2f& mousePosition, Vec2f delta, bool precise, bool momentum) {
    if (!m_bounds.contains(mousePosition)) return false;

    // Offer the full 2D delta to children first (e.g. canvas, nested columns).
    // We intentionally do NOT short-circuit on their return value: a parent
    // Column consuming vertical scroll must not prevent this Row from also
    // handling horizontal scroll on the same event.
    bool consumed = false;
    for (auto* child : m_children)
        if (child->getBounds().contains(mousePosition))
            if (child->checkScroll(mousePosition, delta, precise, momentum)) { consumed = true; break; }

    // Row owns delta.x — always apply horizontal scroll independently.
    if (m_options.getScrollable() && delta.x != 0.f) {
        const float speed      = m_options.getScrollSpeed();
        const float step       = precise ? 30.f * (speed / 40.f) : speed;
        const float contentMax = std::max(0.f, m_contentWidth - m_scrollViewportWidth);
        if (canUseLooseScrollBounds(m_options, contentMax)) {
            float minScroll = 0.f, maxScroll = 0.f;
            resolveScrollBounds(m_options, contentMax, minScroll, maxScroll);
            m_scrollOffset = std::clamp(m_scrollOffset - delta.x * step, minScroll, maxScroll);
            if (m_uiloRef && !m_options.getScrollLink().empty())
                m_uiloRef->setScrollLinkOffset(m_options.getScrollLink(), m_scrollOffset, true);
            if (m_uiloRef && !m_options.getZoomLink().empty())
                m_uiloRef->setZoomLinkValue(m_options.getZoomLink(), m_zoomX, true);
            m_dirty  = true;
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