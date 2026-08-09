/*
    layout_probe.cpp:
    - Desc: Layout-equivalence probe for Row and Column. Drives their update()
            directly with no UILO bound, so it needs no window and no GPU, and
            dumps every resolved child bound plus the container's scroll state.
            Run it before and after a layout change and diff the output: for a
            pure refactor it must be byte-identical.
    - It deliberately covers what examples/ does not exercise at all -- the
      scrollable path, pinned children, resizers in both paths, percent and
      fixed mixes, every alignment bucket, and degenerate slots.
    - Not wired into CMake. Build it against the static lib, e.g.
          clang++ -std=c++20 -I include -I include/elements \
              tools/layout_probe.cpp lib/libuilo.a <platform libs> -o layout_probe
      See TODO.txt, Phase 2, for how it is meant to be used.
*/

#include "elements/Factory.hpp"
#include <cstdio>
#include <string>
#include <vector>

using namespace uilo;

static int g_case = 0;

static void dumpEl(const std::string& tag, Element* e, int depth) {
    Rectf b = e->getBounds();
    std::printf("%*s%-14s pos=(%.4f,%.4f) size=(%.4f,%.4f) vis=%d\n",
                depth * 2, "", tag.c_str(),
                b.position.x, b.position.y, b.size.x, b.size.y,
                (int)e->getModifier().getVisible() ? 1 : 0);
}

static void dumpTree(Element* e, int depth) {
    const char* kind = "el";
    switch (e->getType()) {
        case ElementType::Row:     kind = "Row";     break;
        case ElementType::Column:  kind = "Column";  break;
        case ElementType::Spacer:  kind = "Spacer";  break;
        case ElementType::Resizer: kind = "Resizer"; break;
        default: break;
    }
    dumpEl(kind, e, depth);
    if (auto* c = dynamic_cast<Container*>(e))
        for (auto* ch : c->getChildren()) dumpTree(ch, depth + 1);
}

// Ticks a subtree repeatedly, so any state that settles over frames (scroll
// clamping, content measurement) is captured after it has converged.
static void runCase(const char* name, Element* root, Rectf parent, int frames = 3) {
    std::printf("\n=== case %d: %s ===\n", ++g_case, name);
    for (int f = 0; f < frames; ++f) {
        Rectf p = parent;
        root->tick(p, 1.f / 60.f);
    }
    dumpTree(root, 0);
}

static Spacer* sp(Dimension w, Dimension h, Align a = Align::Left | Align::Top) {
    return spacer(Modifier().setWidth(w).setHeight(h).setAlign(a));
}

int main() {
    const Rectf parent{{0.f, 0.f}, {1000.f, 600.f}};

    /* Plain flow row: fixed + percent mix across all three alignment buckets. */
    runCase("row flow: fixed+percent, all buckets",
        row(Modifier().setWidth(100_pct).setHeight(100_pct), RowOptions(), contains{
            sp(100_px, 100_pct, Align::Left  | Align::Top),
            sp(30_pct, 50_pct,  Align::Left  | Align::CenterY),
            sp(120_px, 100_pct, Align::CenterX | Align::Top),
            sp(20_pct, 100_pct, Align::CenterX | Align::Bottom),
            sp(80_px,  100_pct, Align::Right | Align::Top),
            sp(10_pct, 100_pct, Align::Right | Align::CenterY),
        }), parent);

    /* Same for a column, so the mirrored axis is covered. */
    runCase("column flow: fixed+percent, all buckets",
        column(Modifier().setWidth(100_pct).setHeight(100_pct), ColumnOptions(), contains{
            sp(100_pct, 80_px,  Align::Top    | Align::Left),
            sp(50_pct,  30_pct, Align::Top    | Align::CenterX),
            sp(100_pct, 120_px, Align::CenterY | Align::Left),
            sp(100_pct, 20_pct, Align::CenterY | Align::Right),
            sp(100_pct, 60_px,  Align::Bottom | Align::Left),
            sp(100_pct, 10_pct, Align::Bottom | Align::CenterX),
        }), parent);

    /* Hidden children must take no space and must not shift their siblings. */
    {
        auto* hidden = sp(200_px, 100_pct);
        hidden->getModifier().setVisible(false);
        runCase("row flow: hidden child takes no space",
            row(Modifier().setWidth(100_pct).setHeight(100_pct), RowOptions(), contains{
                sp(100_px, 100_pct), hidden, sp(100_px, 100_pct), sp(50_pct, 100_pct),
            }), parent);
    }

    /* Outer padding shrinks the child inside its own slot without moving siblings. */
    runCase("row flow: outer padding",
        row(Modifier().setWidth(100_pct).setHeight(100_pct), RowOptions(), contains{
            spacer(Modifier().setWidth(200_px).setHeight(100_pct),
                   SpacerOptions().setOuterPadding(12.f)),
            spacer(Modifier().setWidth(200_px).setHeight(100_pct),
                   SpacerOptions().setOuterPadding(0.f)),
            spacer(Modifier().setWidth(30_pct).setHeight(60_pct)
                             .setAlign(Align::CenterX | Align::CenterY),
                   SpacerOptions().setOuterPadding(25.f)),
        }), parent);

    /* Inner padding insets the area children are laid out in, without moving
       the container itself. Both spacers should start 20px in and be 40px
       shorter than the row. */
    runCase("row flow: inner padding",
        row(Modifier().setWidth(100_pct).setHeight(100_pct),
            RowOptions().setInnerPadding(20.f), contains{
            sp(200_px, 100_pct), sp(200_px, 100_pct),
        }), parent);

    /* Inner and outer padding compose: the outer one shrinks each child in its
       own slot, the inner one shrinks the slots. */
    runCase("column flow: inner + outer padding",
        column(Modifier().setWidth(100_pct).setHeight(100_pct),
               ColumnOptions().setInnerPadding(10.f), contains{
            spacer(Modifier().setWidth(100_pct).setHeight(100_px),
                   SpacerOptions().setOuterPadding(5.f)),
            spacer(Modifier().setWidth(100_pct).setHeight(100_px),
                   SpacerOptions()),
        }), parent);

    /* A floating child takes no space in the flow and sits at its free position
       relative to the container's content corner, ignoring its alignment. The two
       non-floating spacers should split the row as if it were not there. */
    runCase("row flow: floating child",
        row(Modifier().setWidth(100_pct).setHeight(100_pct), RowOptions(), contains{
            sp(100_pct, 100_pct),
            spacer(Modifier().setWidth(120_px).setHeight(60_px)
                             .setFloating(true)
                             .setFreePosition(30_px, 40_px)
                             .setAlign(Align::Right | Align::Bottom)),
            sp(100_pct, 100_pct),
        }), parent);

    /* Free position may be a percentage of the container. */
    runCase("column flow: floating child, percent position",
        column(Modifier().setWidth(100_pct).setHeight(100_pct),
               ColumnOptions().setInnerPadding(10.f), contains{
            spacer(Modifier().setWidth(200_px).setHeight(50_px)
                             .setFloating(true)
                             .setFreePosition(50_pct, 25_pct)),
        }), parent);

    /* Scrollable row: overflowing content, offset still at rest. */
    runCase("row scrollable: overflow, offset 0",
        row(Modifier().setWidth(100_pct).setHeight(100_pct),
            RowOptions().setScrollable(true), contains{
            sp(400_px, 100_pct), sp(400_px, 100_pct), sp(400_px, 100_pct), sp(400_px, 100_pct),
        }), parent);

    /* Scrollable row with pinned children on both edges and the centre: the
       pinned strips reserve width and only the gap between them scrolls. */
    {
        auto* pinL = spacer(Modifier().setWidth(90_px).setHeight(100_pct)
                                      .setAlign(Align::Left).ignoreScroll(true));
        auto* pinR = spacer(Modifier().setWidth(70_px).setHeight(100_pct)
                                      .setAlign(Align::Right).ignoreScroll(true));
        auto* pinC = spacer(Modifier().setWidth(60_px).setHeight(50_pct)
                                      .setAlign(Align::CenterX).ignoreScroll(true));
        runCase("row scrollable: pinned left/mid/right",
            row(Modifier().setWidth(100_pct).setHeight(100_pct),
                RowOptions().setScrollable(true), contains{
                pinL, sp(500_px, 100_pct), pinC, sp(500_px, 100_pct), pinR,
                sp(300_px, 100_pct),
            }), parent);
    }

    /* Same shape on the column, covering the pinned top/mid/bottom groups. */
    {
        auto* pinT = spacer(Modifier().setWidth(100_pct).setHeight(90_px)
                                      .setAlign(Align::Top).ignoreScroll(true));
        auto* pinB = spacer(Modifier().setWidth(100_pct).setHeight(70_px)
                                      .setAlign(Align::Bottom).ignoreScroll(true));
        auto* pinC = spacer(Modifier().setWidth(50_pct).setHeight(60_px)
                                      .setAlign(Align::CenterY).ignoreScroll(true));
        runCase("column scrollable: pinned top/mid/bottom",
            column(Modifier().setWidth(100_pct).setHeight(100_pct),
                   ColumnOptions().setScrollable(true), contains{
                pinT, sp(100_pct, 300_px), pinC, sp(100_pct, 300_px), pinB,
                sp(100_pct, 200_px),
            }), parent);
    }

    /* Explicit scroll bounds, which override the content-derived range. */
    runCase("column scrollable: explicit scroll min/max",
        column(Modifier().setWidth(100_pct).setHeight(100_pct),
               ColumnOptions().setScrollable(true).setScrollMin(-50.f).setScrollMax(500.f),
               contains{ sp(100_pct, 200_px), sp(100_pct, 200_px) }), parent);

    /* Content that fits: no overflow, so nothing to scroll into. */
    runCase("column scrollable: content fits",
        column(Modifier().setWidth(100_pct).setHeight(100_pct),
               ColumnOptions().setScrollable(true), contains{
            sp(100_pct, 100_px), sp(100_pct, 100_px),
        }), parent);

    /* Resizers in the plain flow path: placed at the boundary between their
       neighbours, taking no space from the flow. */
    runCase("row flow: resizers between children",
        row(Modifier().setWidth(100_pct).setHeight(100_pct), RowOptions(), contains{
            sp(200_px, 100_pct),
            resizer(Modifier().setWidth(8_px).setHeight(100_pct),
                    ResizerOptions().setDirection(ResizerDir::Left)),
            sp(300_px, 100_pct),
            resizer(Modifier().setWidth(12_px).setHeight(100_pct),
                    ResizerOptions().setDirection(ResizerDir::Right)),
            sp(40_pct, 80_pct),
        }), parent);

    /* Resizers in the scrollable path, which is a separate code path. */
    runCase("row scrollable: resizers between children",
        row(Modifier().setWidth(100_pct).setHeight(100_pct),
            RowOptions().setScrollable(true), contains{
            sp(400_px, 100_pct),
            resizer(Modifier().setWidth(8_px).setHeight(100_pct),
                    ResizerOptions().setDirection(ResizerDir::Left)),
            sp(400_px, 100_pct),
        }), parent);

    /* Column resizers exercise the cross-axis width clamp and alignment that
       the row's resizer path does not currently have. */
    runCase("column flow: resizers, cross-axis align",
        column(Modifier().setWidth(100_pct).setHeight(100_pct), ColumnOptions(), contains{
            sp(100_pct, 150_px),
            resizer(Modifier().setWidth(50_pct).setHeight(10_px)
                              .setAlign(Align::CenterX),
                    ResizerOptions().setDirection(ResizerDir::Top)),
            sp(100_pct, 150_px),
            resizer(Modifier().setWidth(120_px).setHeight(10_px).setAlign(Align::Right),
                    ResizerOptions().setDirection(ResizerDir::Bottom)),
            sp(100_pct, 150_px),
            resizer(Modifier().setWidth(80_px).setHeight(6_px).setAlign(Align::Left),
                    ResizerOptions().setDirection(ResizerDir::Bottom)),
            sp(100_pct, 40_pct),
        }), parent);

    /* A resizer with no neighbour on one or both sides falls back to the
       container edge / centre. */
    runCase("row flow: resizer at edges, no neighbours",
        row(Modifier().setWidth(100_pct).setHeight(100_pct), RowOptions(), contains{
            resizer(Modifier().setWidth(10_px).setHeight(100_pct),
                    ResizerOptions().setDirection(ResizerDir::Left)),
            sp(200_px, 100_pct),
            resizer(Modifier().setWidth(10_px).setHeight(100_pct),
                    ResizerOptions().setDirection(ResizerDir::Right)),
        }), parent);

    runCase("column flow: lone resizer, no neighbours at all",
        column(Modifier().setWidth(100_pct).setHeight(100_pct), ColumnOptions(), contains{
            resizer(Modifier().setWidth(100_pct).setHeight(10_px),
                    ResizerOptions().setDirection(ResizerDir::Top)),
        }), parent);

    /* Nested row-in-column-in-row, so slot propagation is covered. */
    runCase("nested: row > column > row",
        row(Modifier().setWidth(100_pct).setHeight(100_pct), RowOptions(), contains{
            sp(150_px, 100_pct),
            column(Modifier().setWidth(50_pct).setHeight(100_pct), ColumnOptions(), contains{
                sp(100_pct, 100_px),
                row(Modifier().setWidth(100_pct).setHeight(50_pct), RowOptions(), contains{
                    sp(33_pct, 100_pct), sp(33_pct, 100_pct), sp(34_pct, 100_pct),
                }),
                sp(100_pct, 80_px),
            }),
            column(Modifier().setWidth(100_pct).setHeight(100_pct),
                   ColumnOptions().setScrollable(true), contains{
                sp(100_pct, 250_px), sp(100_pct, 250_px), sp(100_pct, 250_px),
            }),
        }), parent);

    /* Degenerate parents: zero and tiny slots must not produce NaNs. */
    runCase("degenerate: zero-size parent",
        row(Modifier().setWidth(100_pct).setHeight(100_pct), RowOptions(), contains{
            sp(100_px, 100_pct), sp(50_pct, 100_pct),
        }), Rectf{{0.f, 0.f}, {0.f, 0.f}});

    runCase("degenerate: fixed children overflow parent",
        row(Modifier().setWidth(100_pct).setHeight(100_pct), RowOptions(), contains{
            sp(800_px, 100_pct), sp(800_px, 100_pct), sp(50_pct, 100_pct),
        }), parent);

    /* Percent children with no fixed siblings, and a lone percent child. */
    runCase("row flow: percent only, sums over 100",
        row(Modifier().setWidth(100_pct).setHeight(100_pct), RowOptions(), contains{
            sp(60_pct, 100_pct), sp(60_pct, 100_pct), sp(60_pct, 100_pct),
        }), parent);

    std::printf("\ncases: %d\n", g_case);
    return 0;
}
