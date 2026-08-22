/*
    ctxmenu_probe.cpp:
    - Desc: Context-menu probe. Unlike layout_probe and pointer_probe this one
            needs a window and a GPU: a menu measures its own labels to size
            itself and flips against the window edges, so a real Renderer and a
            real UILO are the point rather than an obstacle.
    - Right-clicks are driven straight into the tree instead of through SDL, so
      the run is deterministic and needs no input.
    - Prints one line per assertion and exits non-zero if any failed.
    - Not wired into CMake; build it with tools/probe.sh ctxmenu.
    - The Metal "Command encoder released without endEncoding" assertion on exit
      is this harness tearing the renderer down outside a begin/end frame pair,
      not anything the library does in a normal loop.
*/

#include "../include/UILO.hpp"
#include "../include/renderer/Renderer.hpp"
#include <cstdio>
#include <utility>
#include <string>

using namespace uilo;

static int fails = 0;
static void check(const char* what, bool ok, const std::string& detail = "") {
    if (!ok) ++fails;
    std::printf("  %-4s %-46s %s\n", ok ? "ok" : "FAIL", what, detail.c_str());
}
static std::string rect(Rectf b) {
    char buf[96];
    std::snprintf(buf, sizeof buf, "[%.0f,%.0f %.0fx%.0f]",
                  b.position.x, b.position.y, b.size.x, b.size.y);
    return buf;
}

int main() {
    Renderer renderer;
    if (!renderer.init(1000, 700, "ctxmenu test", 8)) { std::puts("no renderer"); return 1; }

    int cut = 0, paste = 0, audio = 0;
    bool haveSelection = false;

    UILO ui;
    ui.setRenderer(renderer);

    Row* lane = row(
        Modifier()
            .setWidth(1_flex)
            /* Builder form: the items depend on state. */
            .setContextMenu([&] {
                std::vector<ContextMenuItem> items {
                    menuItem("Cut", [&] { ++cut; }),
                    menuItem("Paste", [&] { ++paste; }),
                    menuSeparator(),
                    menuSubmenu("Insert", {
                        menuItem("Audio Track", [&] { ++audio; }),
                        menuItem("MIDI Track", [] {}),
                    }),
                };
                if (haveSelection) items.push_back(menuItem("Delete", [] {}));
                return items;
            }),
        RowOptions());

    /* A plain sibling with no menu of its own. */
    Row* bare = row(Modifier().setWidth(200_px), RowOptions());
    /* bare sits under the lane, which now takes the rest. */

    /* A menu-less strip on the left, and a lane filling the rest -- so the
       lane reaches both the top-left area and the bottom-right corner. */
    Row* root = row(Modifier(), RowOptions(), contains{ bare, lane });
    ui.addPage(page(root, "main"));
    ui.setPage("main");
    ui.update();

    std::printf("lane=%s bare=%s\n", rect(lane->getBounds()).c_str(),
                rect(bare->getBounds()).c_str());

    /* --- right-click opens the menu, populated by the element clicked --- */
    const Vec2f onLane{600.f, 80.f};
    check("no menu before any right-click", !ui.isContextMenuOpen());
    root->checkRightClick(onLane);
    check("right-click on the lane opens a menu", ui.isContextMenuOpen());

    ContextMenu* menu = ui.contextMenu();
    Rectf mb = menu->getBounds();
    check("menu is placed at the cursor",
          std::abs(mb.position.x - onLane.x) < 0.6f && std::abs(mb.position.y - onLane.y) < 0.6f,
          rect(mb));
    check("menu has width and height", mb.size.x > 0.f && mb.size.y > 0.f, rect(mb));

    /* --- an element with no menu does not open one --- */
    ui.closeContextMenu();
    root->checkRightClick({100.f, 300.f});
    check("right-click where nothing declares a menu opens nothing",
          !ui.isContextMenuOpen());

    /* --- the builder is re-run, so state changes show up --- */
    haveSelection = true;
    root->checkRightClick(onLane);
    const float withDelete = menu->getBounds().size.y;
    ui.closeContextMenu();
    haveSelection = false;
    root->checkRightClick(onLane);
    const float without = menu->getBounds().size.y;
    check("builder re-runs: extra item makes the menu taller", withDelete > without,
          "with=" + std::to_string((int)withDelete) + " without=" + std::to_string((int)without));

    /* --- flipping at the window edges --- */
    ui.closeContextMenu();
    const auto win = ui.getWindowSize();
    const Vec2f corner{ (float)win.x - 4.f, (float)win.y - 4.f };
    root->checkRightClick(corner);
    check("a corner right-click opened a menu", ui.isContextMenuOpen());
    Rectf flipped = menu->getBounds();
    check("flips back inside at the bottom-right corner",
          flipped.position.x + flipped.size.x <= (float)win.x + 0.5f
       && flipped.position.y + flipped.size.y <= (float)win.y + 0.5f, rect(flipped));
    check("and flipping actually moved it off the cursor",
          flipped.position.x < corner.x - 1.f && flipped.position.y < corner.y - 1.f,
          rect(flipped) + " cursor=" + std::to_string((int)corner.x) + ","
                        + std::to_string((int)corner.y));

    /* --- keyboard navigation skips the separator ---
       Re-opened at onLane, well away from any edge: a submenu opened at the
       bottom-right corner flips to the LEFT of its parent, which is correct but
       not what this case is checking. */
    ui.closeContextMenu();
    root->checkRightClick(onLane);
    check("re-opened away from the edges", ui.isContextMenuOpen());
    check("down highlights the first item", menu->moveHighlight(+1));
    check("down again", menu->moveHighlight(+1));
    /* Third down must land on the submenu row, having stepped over the rule. */
    check("down steps over the separator", menu->moveHighlight(+1));
    check("right opens the submenu", menu->openHighlightedSubmenu());
    check("the submenu is the deepest open", menu->deepestOpen() != menu);
    ContextMenu* sub = menu->openSubmenu();
    check("submenu placed beside its parent row",
          sub && sub->getBounds().position.x > menu->getBounds().position.x,
          sub ? rect(sub->getBounds()) : "none");

    /* --- picking an item in the submenu runs it and closes everything --- */
    check("down highlights inside the submenu", menu->moveHighlight(+1));
    check("enter activates", menu->activateHighlighted());
    check("the submenu action ran", audio == 1, "audio=" + std::to_string(audio));
    check("the whole chain closed", !ui.isContextMenuOpen() && !sub->isOpen());

    /* --- clicking a row runs that row's action --- */
    root->checkRightClick(onLane);
    Rectf firstRow{};
    for (Element* child : menu->getChildren())
        if (child->getModifier().getVisible() && child->getBounds().size.y > 0.f) {
            firstRow = child->getBounds(); break;
        }
    const Vec2f onFirstRow{ firstRow.position.x + 10.f, firstRow.position.y + firstRow.size.y * 0.5f };
    menu->checkLeftClick(onFirstRow);
    check("clicking the first row ran Cut", cut == 1, "cut=" + std::to_string(cut));
    check("and closed the menu", !ui.isContextMenuOpen());

    /* --- a disabled item keeps its place but cannot be reached --- */
    lane->getModifier().setContextMenu({
        menuItem("Enabled A", [] {}),
        menuItem("Disabled",  [] {}).setEnabled(false),
        menuItem("Enabled B", [] {}),
    });
    ui.closeContextMenu();
    root->checkRightClick(onLane);
    menu->moveHighlight(+1);
    menu->moveHighlight(+1);
    /* Two steps from nothing lands on Enabled B, not on Disabled. */
    check("keyboard steps over a disabled item", menu->activateHighlighted());

    /* --- escape closes --- */
    ui.closeContextMenu();
    root->checkRightClick(onLane);
    check("open again", ui.isContextMenuOpen());
    ui.closeContextMenu();
    check("closeContextMenu closes", !ui.isContextMenuOpen());

    /* --- the pool grows and is reused, never rebuilt --- */
    const size_t poolAfter = menu->getChildren().size();
    root->checkRightClick(onLane);
    check("reopening does not add children", menu->getChildren().size() == poolAfter,
          std::to_string(menu->getChildren().size()) + " vs " + std::to_string(poolAfter));

    /*
        A submenu at the right edge has to open to the LEFT of its parent, not on
        top of it. Flipping about the row's right edge -- the obvious reading of
        "does not fit" -- lands the submenu almost exactly over its parent and
        buries every row below the one that opened it.
    */
    ui.closeContextMenu();
    lane->getModifier().setContextMenu({
        menuItem("First", [] {}),
        menuSubmenu("Opens a submenu", {
            menuItem("Child one", [] {}),
            menuItem("Child two", [] {}),
        }),
        menuItem("Below the submenu row", [] {}),
    });
    auto openSubmenuAt = [&](Vec2f at) -> std::pair<Rectf, Rectf> {
        ui.closeContextMenu();
        root->checkRightClick(at);
        menu->moveHighlight(+1);
        menu->moveHighlight(+1);                    /* the submenu row */
        menu->openHighlightedSubmenu();
        ContextMenu* sub = menu->openSubmenu();
        return { menu->getBounds(), sub ? sub->getBounds() : Rectf{} };
    };

    {
        const auto w = ui.getWindowSize();

        /* Room to the right: the submenu sits just inside the parent's right
           edge, overlapping by the inner padding plus the configured overlap. */
        auto [pRight, sRight] = openSubmenuAt({ 200.f, 200.f });
        const float overlapOnRight =
            (pRight.position.x + pRight.size.x) - sRight.position.x;
        check("submenu opens to the right when there is room",
              sRight.position.x > pRight.position.x,
              "parent " + rect(pRight) + " sub " + rect(sRight));

        /* No room: it has to mirror -- its right edge inside the parent's LEFT
           edge by the same amount. Flipping about the row's right edge instead
           would drop it on top of the parent and bury the rows below. */
        auto [pEdge, sEdge] = openSubmenuAt({ (float)w.x - 6.f, 200.f });
        const float overlapOnLeft =
            (sEdge.position.x + sEdge.size.x) - pEdge.position.x;

        check("at the edge the submenu opens to the left",
              sEdge.position.x < pEdge.position.x,
              "parent " + rect(pEdge) + " sub " + rect(sEdge));
        check("and it mirrors the right-hand overlap exactly",
              std::abs(overlapOnLeft - overlapOnRight) < 0.51f,
              "right=" + std::to_string((int)overlapOnRight)
            + " left=" + std::to_string((int)overlapOnLeft));
        check("submenu stays on screen", sEdge.position.x >= -0.5f, rect(sEdge));
        check("submenu stays level with its row",
              sEdge.position.y >= pEdge.position.y - 0.5f, rect(sEdge));
    }
    ui.closeContextMenu();

    /*
        Picking two levels down. Activating a row that is itself a submenu OPENS
        it rather than picking it, so reaching a leaf takes one activate per
        level -- easy to get wrong, and the reason this is pinned down.
    */
    ui.closeContextMenu();
    lane->getModifier().setContextMenu({
        menuSubmenu("Outer", {
            menuItem("Near", [] {}),
            menuSubmenu("Inner", { menuItem("Far", [&] { ++audio; }) }),
        }),
    });
    audio = 0;
    root->checkRightClick(onLane);
    menu->moveHighlight(+1);                       /* Outer */
    check("activating Outer opens it", menu->activateHighlighted());
    menu->moveHighlight(+1);                       /* Near */
    menu->moveHighlight(+1);                       /* Inner */
    check("activating Inner opens it too", menu->activateHighlighted());
    menu->moveHighlight(+1);                       /* Far */
    check("activating Far runs it", menu->activateHighlighted());
    check("the two-level action ran", audio == 1, "audio=" + std::to_string(audio));
    check("and the whole chain closed", !ui.isContextMenuOpen());

    /* Restore the menu the later cases expect. */
    lane->getModifier().setContextMenu([&] {
        return std::vector<ContextMenuItem>{
            menuItem("Cut", [&] { ++cut; }),
            menuItem("Paste", [&] { ++paste; }),
            menuSeparator(),
            menuSubmenu("Insert", { menuItem("Audio Track", [&] { ++audio; }) }),
        };
    });

    /*
        The labels have to be renderable, not merely stored. A Text keeps a live
        string separate from its options and only reads the options' content when
        it is constructed, so setting the options after the fact leaves the row
        blank on screen while still reading back correctly through
        getOptions().getContent() -- which is why this asserts on getString().
    */
    ui.closeContextMenu();
    root->checkRightClick(onLane);
    {
        int labelled = 0, blank = 0;
        for (Element* rowEl : menu->getChildren()) {
            if (!rowEl->getModifier().getVisible()) continue;
            auto* asRow = dynamic_cast<Container*>(rowEl);
            if (!asRow) continue;                 /* a separator rule */
            for (Element* part : asRow->getChildren())
                if (auto* t = dynamic_cast<Text*>(part))
                    (t->getString().empty() ? blank : labelled)++;
        }
        check("every visible row has a renderable label",
              labelled > 0 && blank == 0,
              std::to_string(labelled) + " labelled, " + std::to_string(blank) + " blank");
    }

    /*
        The path a real trackpad takes: SDL delivers the press as an event, and a
        tap's press and release can both be drained in one pollEvents() -- so by
        the time update() polls the button state it reads as up. A press latched
        out of the event stream is the only way that click survives. Pushed
        events do not change what SDL_GetMouseState() reports, which is exactly
        the failing shape.
    */
    ui.closeContextMenu();
    {
        SDL_Event down{};
        down.type         = SDL_EVENT_MOUSE_BUTTON_DOWN;
        down.button.button = SDL_BUTTON_RIGHT;
        down.button.down   = true;
        down.button.clicks = 1;
        down.button.windowID = SDL_GetWindowID(renderer.sdlWindow());
        /* Event coordinates are window points; the lane covers most of the
           window, so a point well inside it converts to a pixel position inside
           the lane whatever the backing scale. */
        down.button.x = 400.f;
        down.button.y = 300.f;

        SDL_Event up = down;
        up.type        = SDL_EVENT_MOUSE_BUTTON_UP;
        up.button.down = false;

        SDL_PushEvent(&down);
        SDL_PushEvent(&up);

        ui.pollEvents();     /* drains both, so the polled state stays "up" */
        ui.update();
    }
    check("a tap whose press and release land in one frame still opens a menu",
          ui.isContextMenuOpen());
    if (ui.isContextMenuOpen()) {
        Rectf tapped = menu->getBounds();
        check("and it opens at the press position, not the cursor",
              tapped.position.x > 0.f && tapped.position.y > 0.f, rect(tapped));
    }

    /* One real frame, so the renderer is not destroyed mid-encode. */
    ui.update();
    ui.render();

    std::printf("\n%s\n", fails ? "FAILURES" : "all context-menu assertions pass");
    return fails ? 1 : 0;
}
