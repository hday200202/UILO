/*
    pointer_probe.cpp:
    - Desc: Pointer-event probe. Drives checkHover/checkLeftClick/checkScroll
            directly with no UILO bound, so it needs no window and no GPU, and
            asserts who ended up hovered and whose handlers fired. Its subject is
            occlusion: an element drawn on top of another must take the pointer
            away from it, and layout alone cannot say whether that happens.
    - Prints one line per assertion and exits non-zero if any failed, so it is a
      test rather than a diff-me dump like layout_probe.cpp.
    - Not wired into CMake; build it with tools/probe.sh pointer.
*/

#include "elements/Factory.hpp"
#include <cstdio>
#include <string>

using namespace uilo;

static int g_fail = 0;

static void check(const char* what, bool got, bool want) {
    const bool ok = got == want;
    if (!ok) ++g_fail;
    std::printf("  %-4s %-58s got=%d want=%d\n", ok ? "ok" : "FAIL", what, (int)got, (int)want);
}

static void checkInt(const char* what, int got, int want) {
    const bool ok = got == want;
    if (!ok) ++g_fail;
    std::printf("  %-4s %-58s got=%d want=%d\n", ok ? "ok" : "FAIL", what, got, want);
}

static void heading(const char* name) { std::printf("\n=== %s ===\n", name); }

static void settle(Element* root, Rectf parent, int frames = 3) {
    for (int f = 0; f < frames; ++f) {
        Rectf p = parent;
        root->tick(p, 1.f / 60.f);
    }
}

int main() {
    const Rectf parent{{0.f, 0.f}, {1000.f, 600.f}};

    /*
        A floating panel laid over a button, the shape of the FPS HUD in
        examples/containers.cpp. The panel holds nothing interactive, so nothing
        in it claims the pointer on its own -- being on top has to be enough.
    */
    heading("floating panel over a button");
    int clicks = 0;
    Button* btn = button(
        Modifier()
            .setWidth(300_px)
            .setHeight(200_px)
            .setOnLeftClick([&](Element*) { ++clicks; }),
        ButtonOptions()
    );
    Column* panel = column(
        Modifier()
            .setWidth(150_px)
            .setHeight(100_px)
            .setFloating(true)
            .setFreePosition(50_px, 50_px),
        ColumnOptions(),
        contains{ spacer(Modifier().setHeight(100_pct)) }
    );
    Row* root = row(
        Modifier().setWidth(100_pct).setHeight(100_pct),
        RowOptions(),
        contains{ btn, panel }
    );
    settle(root, parent);

    /* (100,100) is inside both; (250,150) is inside the button only. */
    const Vec2f onPanel{100.f, 100.f};
    const Vec2f onButtonOnly{250.f, 150.f};

    root->checkHover(onPanel);
    check("hover over panel: panel hovered",  panel->isHovered(), true);
    check("hover over panel: button hovered", btn->isHovered(),   false);

    root->checkHover(onButtonOnly);
    check("hover beside panel: panel hovered",  panel->isHovered(), false);
    check("hover beside panel: button hovered", btn->isHovered(),   true);

    root->checkLeftClick(onPanel);
    checkInt("click over panel does not reach the button", clicks, 0);

    root->checkLeftClick(onButtonOnly);
    checkInt("click beside panel reaches the button", clicks, 1);

    /* A floating panel that scrolls nothing still swallows the wheel, or the
       view behind it would slide out from under the pointer. */
    int scrolls = 0;
    root->getModifier().setOnScroll([&](Element*, float) { ++scrolls; });
    root->checkScroll(onPanel, -3.f, false, false);
    checkInt("scroll over panel does not reach the container", scrolls, 0);
    root->checkScroll(onButtonOnly, -3.f, false, false);
    checkInt("scroll beside panel reaches the container", scrolls, 1);

    /*
        An invisible floating panel is not there at all: the button under it must
        behave as if the panel had never been added.
    */
    heading("hidden floating panel does not occlude");
    panel->getModifier().setVisible(false);
    settle(root, parent);
    root->checkHover(onPanel);
    check("hover through hidden panel: button hovered", btn->isHovered(), true);
    clicks = 0;
    root->checkLeftClick(onPanel);
    checkInt("click through hidden panel reaches the button", clicks, 1);
    panel->getModifier().setVisible(true);

    /*
        A Knob and a Slider ask for a resize cursor from checkHover, so the only
        way the cursor stops changing under a panel is for them not to be told
        where the pointer is. Their hovered flag is the observable side of that
        same call.
    */
    heading("knob and slider under a floating panel");
    Knob* kn = knob(
        Modifier().setWidth(100_px).setHeight(100_px),
        KnobOptions()
    );
    Slider* sl = slider(
        Modifier().setWidth(100_px).setHeight(100_px),
        SliderOptions()
    );
    Column* cover = column(
        Modifier()
            .setWidth(200_px)
            .setHeight(200_px)
            .setFloating(true)
            .setFreePosition(0_px, 0_px),
        ColumnOptions()
    );
    Row* dials = row(
        Modifier().setWidth(100_pct).setHeight(100_pct),
        RowOptions(),
        contains{ kn, sl, cover }
    );
    settle(dials, parent);

    /* Centre of the knob, which is also inside the panel laid over it. */
    dials->checkHover({50.f, 50.f});
    check("knob under panel hovered", kn->isHovered(), false);
    dials->checkHover({150.f, 50.f});
    check("slider under panel hovered", sl->isHovered(), false);

    cover->getModifier().setVisible(false);
    settle(dials, parent);
    dials->checkHover({50.f, 50.f});
    check("knob hovered once the panel is gone", kn->isHovered(), true);

    /*
        Two floating panels overlapping each other: the later child is drawn on
        top, so it is the one that takes the pointer.
    */
    heading("topmost of two floating panels wins");
    int lowClicks = 0, highClicks = 0;
    Column* low = column(
        Modifier()
            .setWidth(200_px)
            .setHeight(200_px)
            .setFloating(true)
            .setFreePosition(400_px, 100_px)
            .setOnLeftClick([&](Element*) { ++lowClicks; }),
        ColumnOptions()
    );
    Column* high = column(
        Modifier()
            .setWidth(200_px)
            .setHeight(200_px)
            .setFloating(true)
            .setFreePosition(450_px, 150_px)
            .setOnLeftClick([&](Element*) { ++highClicks; }),
        ColumnOptions()
    );
    Row* stack = row(
        Modifier().setWidth(100_pct).setHeight(100_pct),
        RowOptions(),
        contains{ low, high }
    );
    settle(stack, parent);

    const Vec2f onBoth{500.f, 200.f};
    stack->checkHover(onBoth);
    check("overlap: upper hovered", high->isHovered(), true);
    check("overlap: lower hovered", low->isHovered(),  false);
    stack->checkLeftClick(onBoth);
    checkInt("overlap: upper clicked", highClicks, 1);
    checkInt("overlap: lower clicked", lowClicks,  0);

    /*
        A floating child inside a nested container shields only what that
        container holds. A sibling of the container, overlapped by nothing, is
        untouched.
    */
    heading("floating child shields its own container only");
    int deepClicks = 0;
    Button* deep = button(
        Modifier()
            .setWidth(100_pct)
            .setHeight(100_pct)
            .setOnLeftClick([&](Element*) { ++deepClicks; }),
        ButtonOptions()
    );
    Column* pane = column(
        Modifier().setWidth(50_pct).setHeight(100_pct),
        ColumnOptions(),
        contains{
            deep,
            column(
                Modifier()
                    .setWidth(100_px)
                    .setHeight(100_px)
                    .setFloating(true)
                    .setFreePosition(0_px, 0_px),
                ColumnOptions()
            ),
        }
    );
    Button* neighbour = button(
        Modifier().setWidth(50_pct).setHeight(100_pct),
        ButtonOptions()
    );
    Row* split = row(
        Modifier().setWidth(100_pct).setHeight(100_pct),
        RowOptions(),
        contains{ pane, neighbour }
    );
    settle(split, parent);

    split->checkHover({50.f, 50.f});
    check("nested: covered button hovered",  deep->isHovered(),      false);
    check("nested: neighbour hovered",       neighbour->isHovered(), false);
    split->checkLeftClick({50.f, 50.f});
    checkInt("nested: click over floating child is swallowed", deepClicks, 0);

    split->checkHover({700.f, 50.f});
    check("nested: neighbour hovered when pointed at", neighbour->isHovered(), true);
    check("nested: covered button still not hovered",  deep->isHovered(),      false);

    std::printf("\n%s\n", g_fail ? "FAILURES" : "all pointer assertions pass");
    return g_fail ? 1 : 0;
}
