// Context menus: right-click anything.
//
// Nothing here builds, positions or owns a menu. Each element declares what it
// offers with Modifier::setContextMenu, and UILO's one root-held menu shows it
// at the cursor. Both spellings are used:
//
//   * the page background and the clips use a fixed LIST -- their menus never
//     change, so they are declared once;
//   * the lanes use a BUILDER -- it runs on every right-click, so "Clear Lane"
//     is greyed out while the lane is empty and "Delete Last Clip" only appears
//     when there is one.
//
// Try, in a lane:
//   right-click             the lane's menu, built for its current state
//   right-click on a clip   the clip's own menu -- the deepest element with
//                           items wins, so the lane underneath never sees it
//   right-click the bar at the top, or the empty background, for two more
//   arrows                  move the highlight, stepping over separators and
//                           greyed-out rows
//   right / left            enter and leave a submenu
//   enter                   pick; escape backs out one level, then closes
//   right-click near a window edge to watch the menu flip to stay on screen
//
// The status line under the header shows what the last picked item did, so it is
// obvious an action really ran.

#include "../include/UILO.hpp"
#include "../include/renderer/Renderer.hpp"
#include <SDL3/SDL.h>
#include <cstdio>
#include <string>
#include <vector>

using namespace uilo;

using icons = Resources::icons;

constexpr int   kLanes         = 4;
constexpr int   kClipsPerLane  = 6;
constexpr float kRounding      = 6.f;

static Text* g_status = nullptr;

/*
    setStatus(const std::string& text):
    - Params:   const std::string& text
    - Returns:  void
    - Desc:     Writes the line under the header, which is how the example shows
                that a picked item actually ran its action.
*/
static void setStatus(const std::string& text) {
    if (g_status) g_status->setString(text);
}


static Palette darkPalette() {
    Palette p;
    p.set("app.bg",     { 26,  28,  38, 255});
    p.set("panel",      { 38,  41,  54, 255});
    p.set("panelAlt",   { 52,  56,  73, 255});
    p.set("text",       {202, 210, 235, 255});
    p.set("textDim",    {126, 135, 166, 255});
    p.set("accent",     {132, 164, 244, 255});
    p.set("onAccent",   {255, 255, 255, 255});
    p.set("outline",    { 64,  69,  88, 255});
    p.set("lane",       { 32,  35,  46, 255});
    p.set("clip",       { 96, 128, 208, 255});
    p.set("clip.alt",   {188, 122, 190, 255});
    p.set("clip.blue",  { 96, 128, 208, 255});
    p.set("clip.purple",{188, 122, 190, 255});
    p.set("clip.green", { 96, 186, 140, 255});
    p.set("clip.amber", {224, 168,  84, 255});
    return p;
}


/*
    Lane:
    - Desc:     One track's row of clips. The clips are built once and shown or
                hidden, so "Add Clip" and "Delete" never touch the tree -- which
                is also what keeps the count the builder reads cheap to check.
*/
struct Lane {
    Row*               row = nullptr;
    std::vector<Row*>  clips;
    int                visible = 0;

    void refresh() {
        for (int i = 0; i < (int)clips.size(); ++i)
            clips[(size_t)i]->getModifier().setVisible(i < visible);
    }
    void add()    { if (visible < (int)clips.size()) { ++visible; refresh(); } }
    void remove() { if (visible > 0)                { --visible; refresh(); } }
    void clear()  { visible = 0; refresh(); }
};

static Lane g_lanes[kLanes];


/*
    recolorClip(int lane, int index, const std::string& role, const std::string& name):
    - Params:   int lane, int index, const std::string& role, const std::string&
                name
    - Returns:  void
    - Desc:     Points a clip at a different palette role. A container resolves
                its role every time it draws, so this shows on the next frame with
                nothing rebuilt and no need to mark anything dirty.
*/
static void recolorClip(
    int lane,
    int index,
    const std::string& role,
    const std::string& name
) {
    Lane& l = g_lanes[lane];
    if (index < 0 || index >= (int)l.clips.size()) return;
    l.clips[(size_t)index]->getOptions().setColorRole(role);
    setStatus(name + ": clip " + std::to_string(index + 1)
            + " on track " + std::to_string(lane + 1));
}


/*
    clipMenu(int lane, int index):
    - Params:   int lane, int index
    - Returns:  std::vector<ContextMenuItem>
    - Desc:     A clip's own menu, as a fixed list: it is the same whatever the
                clip is doing. Declared on the clip, so right-clicking one never
                reaches the lane behind it.
*/
static std::vector<ContextMenuItem> clipMenu(int lane, int index) {
    const std::string what = "clip " + std::to_string(index + 1)
                           + " on track " + std::to_string(lane + 1);
    return {
        menuItem("Cut",   icons::scissors,  [what] { setStatus("Cut "   + what); }),
        menuItem("Copy",  icons::copy,      [what] { setStatus("Copy "  + what); }),
        menuSeparator(),
        menuSubmenu("Color", icons::droplet, {
            menuItem("Blue",   [lane, index] { recolorClip(lane, index, "clip.blue",   "Blue"); }),
            menuItem("Purple", [lane, index] { recolorClip(lane, index, "clip.purple", "Purple"); }),
            menuSubmenu("More", {
                menuItem("Green", [lane, index] { recolorClip(lane, index, "clip.green", "Green"); }),
                menuItem("Amber", [lane, index] { recolorClip(lane, index, "clip.amber", "Amber"); }),
            }),
        }),
        menuSeparator(),
        menuItem("Delete", icons::trash_2, [lane, what] {
            g_lanes[lane].remove();
            setStatus("Deleted " + what);
        }),
    };
}


/*
    laneMenu(int lane):
    - Params:   int lane
    - Returns:  std::vector<ContextMenuItem>
    - Desc:     A lane's menu, rebuilt on every right-click. The items depend on
                how full the lane is, which is the case the builder form exists
                for: a fixed list would have to be re-declared every time a clip
                was added or removed.
*/
static std::vector<ContextMenuItem> laneMenu(int lane) {
    Lane& l = g_lanes[lane];
    const bool empty = l.visible == 0;
    const bool full  = l.visible >= (int)l.clips.size();

    std::vector<ContextMenuItem> items {
        menuItem("Add Clip", icons::plus, [lane] {
            g_lanes[lane].add();
            setStatus("Added a clip to track " + std::to_string(lane + 1));
        }).setEnabled(!full),
        menuItem("Paste", icons::clipboard, [lane] {
            setStatus("Pasted into track " + std::to_string(lane + 1));
        }),
        menuSeparator(),
        menuSubmenu("Insert", icons::layers, {
            menuItem("Audio Track", icons::music, [] { setStatus("Inserted an audio track"); }),
            menuItem("MIDI Track",  icons::sliders, [] { setStatus("Inserted a MIDI track"); }),
            menuSeparator(),
            menuItem("Send",        [] { setStatus("Inserted a send"); }),
        }),
    };

    /* Only worth offering once there is something to delete, and greyed out
       rather than missing once the lane is empty again -- the two together show
       both ways an item can say "not now". */
    if (!empty) {
        items.push_back(menuSeparator());
        items.push_back(menuItem("Delete Last Clip", [lane] {
            g_lanes[lane].remove();
            setStatus("Deleted the last clip on track " + std::to_string(lane + 1));
        }));
    }
    items.push_back(menuSeparator());
    items.push_back(menuItem("Clear Lane", icons::trash_2, [lane] {
        g_lanes[lane].clear();
        setStatus("Cleared track " + std::to_string(lane + 1));
    }).setEnabled(!empty));

    return items;
}


/*
    buildClip(int lane, int index):
    - Params:   int lane, int index
    - Returns:  Row*
    - Desc:     One clip. It carries its own menu, so it is the deepest element
                under the cursor and the lane never sees the click.
*/
static Row* buildClip(int lane, int index) {
    return row(
        Modifier()
            .setWidth(130_px)
            .setContextMenu(clipMenu(lane, index)),
        RowOptions()
            .setColorRole(index % 2 ? "clip.alt" : "clip")
            .setRounding(kRounding)
            .setOuterPadding(4.f),
        contains{
            text(
                Modifier(),
                TextOptions()
                    .setContent("clip " + std::to_string(index + 1))
                    .setCharSize(13)
                    .setColorRole("onAccent")
                    .setTextAlignX(Align::CenterX)
                    .setTextAlignY(Align::CenterY)
            ),
        }
    );
}


/*
    buildLane(int lane):
    - Params:   int lane
    - Returns:  Row*
    - Desc:     A track: a fixed name plate, then the clips, then the empty space
                that is still part of the lane and still right-clickable.
*/
static Row* buildLane(int lane) {
    Lane& l = g_lanes[lane];

    l.row = row(
        Modifier()
            .setHeight(64_px)
            .setContextMenu([lane] { return laneMenu(lane); }),
        RowOptions()
            .setColorRole("lane")
            .setRounding(kRounding)
            .setOuterPadding(3.f)
            .setInnerPadding(6.f)
    );

    l.row->addElement(
        text(
            Modifier()
                .setWidth(90_px)
                .setAlign(Align::Left | Align::CenterY),
            TextOptions()
                .setContent("Track " + std::to_string(lane + 1))
                .setCharSize(14)
                .setColorRole("textDim")
                .setTextAlignX(Align::Left)
                .setTextAlignY(Align::CenterY)
        )
    );

    for (int i = 0; i < kClipsPerLane; ++i) {
        Row* clip = buildClip(lane, i);
        l.clips.push_back(clip);
        l.row->addElement(clip);
    }

    /* Takes the rest of the lane, so the empty part answers a right-click with
       the lane's menu instead of falling through to the page. */
    l.row->addElement(spacer(Modifier().setWidth(1_flex)));

    l.visible = lane + 1;
    l.refresh();
    return l.row;
}


static Container* buildRoot() {
    Column* timeline = column(
        Modifier(),
        ColumnOptions()
            .setColorRole("panel")
            .setRounding(kRounding)
            .setInnerPadding(8.f)
    );
    for (int i = 0; i < kLanes; ++i) timeline->addElement(buildLane(i));

    g_status = text(
        Modifier()
            .setHeight(26_px),
        TextOptions()
            .setContent("Right-click a lane, a clip, the bar, or the background.")
            .setCharSize(14)
            .setColorRole("accent")
            .setTextAlignX(Align::Left)
            .setTextAlignY(Align::CenterY)
    );

    return column(
        /* The page background's own menu: the fallback for a right-click that
           lands on nothing more specific. */
        Modifier()
            .setContextMenu({
                menuItem("New Project",  icons::file,     [] { setStatus("New project"); }),
                menuItem("Open Project", icons::folder,   [] { setStatus("Opened a project"); }),
                menuSeparator(),
                menuSubmenu("View", {
                    menuItem("Zoom In",  [] { setStatus("Zoom in"); }),
                    menuItem("Zoom Out", [] { setStatus("Zoom out"); }),
                }),
                menuSeparator(),
                menuItem("Preferences", icons::settings, [] { setStatus("Preferences"); }),
            }),
        ColumnOptions()
            .setColorRole("app.bg")
            .setInnerPadding(16.f),
        contains{
            text(
                Modifier()
                    .setHeight(34_px),
                TextOptions()
                    .setContent("Context menus")
                    .setCharSize(26)
                    .setColorRole("text")
                    .setTextAlignX(Align::Left)
                    .setTextAlignY(Align::CenterY)
            ),
            g_status,

            spacer(Modifier().setHeight(10_px)),

            /* A transport bar with a menu of its own, to show a third distinct
               menu on an element that is neither a lane nor a clip. */
            row(
                Modifier()
                    .setHeight(44_px)
                    .setContextMenu({
                        menuItem("Play",   icons::play,  [] { setStatus("Play"); }),
                        menuItem("Record", icons::mic,   [] { setStatus("Record"); }),
                        menuSeparator(),
                        menuItem("Metronome", [] { setStatus("Metronome toggled"); }),
                        menuItem("Loop",      [] { setStatus("Loop toggled"); })
                            .setEnabled(false),
                    }),
                RowOptions()
                    .setColorRole("panelAlt")
                    .setRounding(kRounding)
                    .setInnerPadding(10.f),
                contains{
                    text(
                        Modifier()
                            .setAlign(Align::Left | Align::CenterY),
                        TextOptions()
                            .setContent("transport  --  right-click me too")
                            .setCharSize(14)
                            .setColorRole("textDim")
                            .setTextAlignX(Align::Left)
                            .setTextAlignY(Align::CenterY)
                    ),
                }
            ),

            spacer(Modifier().setHeight(10_px)),
            timeline,
        },
        "root"
    );
}


int main() {
    Renderer renderer;
    if (!renderer.init(1100, 640, "Context Menus", 16)) {
        std::fprintf(stderr, "Failed to initialize renderer\n");
        return 1;
    }

    UILO ui;
    ui.setRenderer(renderer);
    ui.setPalette(darkPalette());
    ui.addPage(page(buildRoot(), "main"));
    ui.setPage("main");
    ui.setScale(OS::scale());

    /* The menu follows the palette with nothing named here; these are only to
       show that it is configurable. */
    ui.contextMenu()->getOptions()
        .setItemHeight(30.f)
        .setMinWidth(190.f);

    while (ui.isRunning()) {
        ui.pollEvents();
        ui.update();

        renderer.beginFrame();
        renderer.clear(ui.getPalette().get("app.bg"));
        ui.render();
        renderer.endFrame();
    }
    return 0;
}
