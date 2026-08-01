// Theme: application-wide defaults, and what overrides them.
//
// Nothing below names a colour role, a corner radius or a font. Every element
// and every widget follows Theme::current() on its own -- the dropdown, the
// textbox and the date field are built with nothing but a placeholder. The rows
// on the right override the theme, to show a manual setting still wins. Borders
// are never themed, so the two rows that have one asked for it.
//
// The buttons across the top change the theme while the app is running. Nothing
// is rebuilt -- themed values are read where they are used, so the tree restyles
// itself on the next frame.

#include "../include/UILO.hpp"
#include "../include/renderer/Renderer.hpp"
#include <SDL3/SDL.h>
#include <cstdio>

using namespace uilo;

static Palette darkPalette() {
    Palette p;
    p.set("app.bg",    { 33,  35,  47, 255});
    p.set("panel",     { 44,  47,  60, 255});
    p.set("panelAlt",  { 55,  58,  74, 255});
    p.set("text",      {180, 190, 220, 255});
    p.set("textDim",   {130, 138, 170, 255});
    p.set("accent",    {151, 120, 206, 255});
    p.set("outline",   { 80,  84, 100, 255});
    p.set("onAccent",  {255, 255, 255, 255});
    return p;
}

static Palette lightPalette() {
    Palette p;
    p.set("app.bg",    {235, 238, 245, 255});
    p.set("panel",     {250, 251, 254, 255});
    p.set("panelAlt",  {220, 225, 238, 255});
    p.set("text",      { 40,  46,  72, 255});
    p.set("textDim",   { 95, 104, 132, 255});
    p.set("accent",    {120,  90, 190, 255});
    p.set("outline",   {180, 186, 205, 255});
    p.set("onAccent",  {255, 255, 255, 255});
    return p;
}

// A labelled button that runs `action`. Takes no colour and no radius: both come
// from the theme.
static Button* themedButton(const std::string& label, std::function<void()> action) {
    return button(
        Modifier()
            .setHeight(36_px)
            .setOnLeftClick([action](Element*) { action(); }),
        ButtonOptions()
            .setColorRole("panelAlt")
            .setLabel(
                text(
                    Modifier()
                        .setWidth(100_pct)
                        .setHeight(100_pct),
                    TextOptions()
                        .setContent(label)
                        .setTextAlignX(Align::CenterX)
                        .setTextAlignY(Align::CenterY)
                )
            )
    );
}

static Element* caption(const std::string& s) {
    return text(
        Modifier()
            .setHeight(24_px),
        TextOptions()
            .setContent(s)
            .setColorRole("textDim")
    );
}

// One of the "Overrides it" rows: a label inside a panel styled however the
// caller says, so each row shows exactly one setting winning over the theme.
static Element* overrideRow(const std::string& label, RowOptions options) {
    return row(
        Modifier()
            .setHeight(56_px),
        options,
        contains {
            spacer(
                Modifier()
                    .setWidth(16_px)
            ),

            text(
                Modifier()
                    .setAlign(Align::Left | Align::CenterY),
                TextOptions()
                    .setContent(label)
                    .setCharSize(14)
                    .setTextAlignY(Align::CenterY)
            )
        }
    );
}

static Element* gap(Dimension height) {
    return spacer(
        Modifier()
            .setHeight(height)
    );
}

Container* buildRoot();

int main() {
    Renderer renderer;
    if (!renderer.init(1100, 760, "Theme", 16)) {
        std::fprintf(stderr, "Failed to initialize renderer\n");
        return 1;
    }

    // The whole look of the app, set once. No element below repeats any of it.
    Theme::current()
        .setPalette(darkPalette())
        .setRounding(8.f)
        .setCharSize(15)
        .setIconStrokeWidth(1.75f);

    UILO ui;
    ui.setRenderer(renderer);
    ui.setScale(OS::scale());
    // Note: no ui.setPalette(). Leaving it unset is what keeps this UILO
    // following the theme, so the palette buttons below take effect live.

    ui.addPage(page(buildRoot(), "main_page"));
    ui.setPage("main_page");

    while (ui.isRunning()) {
        ui.pollEvents();
        ui.update();

        renderer.beginFrame();
        // Reads through the const getPalette(), so it follows the theme live.
        renderer.clear(ui.getPalette().get("app.bg"));
        ui.render();
        renderer.endFrame();
    }
    return 0;
}

Container* buildRoot() {
    return column(
        Modifier(),
        ColumnOptions().setColorRole("app.bg"),
        contains {
            gap(24_px),

            row(
                Modifier()
                    .setHeight(40_px),
                RowOptions(),
                contains {
                    spacer(
                        Modifier()
                            .setWidth(24_px)
                    ),

                    text(
                        Modifier(),
                        TextOptions()
                            .setContent("uilo::Theme")
                            .setCharSize(22)
                            .setBold(true)
                    )
                }
            ),

            gap(16_px),

            // ---- Live theme switches ----
            row(
                Modifier()
                    .setHeight(36_px),
                RowOptions(),
                contains {
                    spacer(
                        Modifier()
                            .setWidth(24_px)
                    ),

                    themedButton("Square", [] {
                        Theme::current().setRounding(0.f);
                    }),

                    spacer(
                        Modifier()
                            .setWidth(8_px)
                    ),

                    themedButton("Rounded", [] {
                        Theme::current().setRounding(8.f);
                    }),

                    spacer(
                        Modifier()
                            .setWidth(8_px)
                    ),

                    themedButton("Pill", [] {
                        Theme::current().setRounding(18.f);
                    }),

                    spacer(
                        Modifier()
                            .setWidth(24_px)
                    ),

                    themedButton("Dark", [] {
                        Theme::current().setPalette(darkPalette());
                    }),

                    spacer(
                        Modifier()
                            .setWidth(8_px)
                    ),

                    themedButton("Light", [] {
                        Theme::current().setPalette(lightPalette());
                    }),

                    spacer(
                        Modifier()
                            .setWidth(100_pct)
                    )
                }
            ),

            gap(24_px),

            row(
                Modifier(),
                RowOptions(),
                contains {
                    spacer(
                        Modifier()
                            .setWidth(24_px)
                    ),

                    // ---- Everything here follows the theme ----
                    column(
                        Modifier()
                            .setWidth(300_px),
                        ColumnOptions(),
                        contains {
                            caption("Follows the theme"),

                            row(
                                Modifier()
                                    .setHeight(56_px),
                                RowOptions()
                                    .setColorRole("panel"),
                                contains {
                                    spacer(
                                        Modifier()
                                            .setWidth(12_px)
                                    ),

                                    icon(
                                        Modifier()
                                            .setWidth(22_px)
                                            .setAlign(Align::CenterY),
                                        IconOptions()
                                            .setIcon(Resources::icons::layers)
                                    ),

                                    spacer(
                                        Modifier()
                                            .setWidth(12_px)
                                    ),

                                    text(
                                        Modifier()
                                            .setAlign(Align::Left | Align::CenterY),
                                        TextOptions()
                                            .setContent("panel, icon, text")
                                            .setTextAlignY(Align::CenterY)
                                    )
                                }
                            ),

                            gap(12_px),

                            textbox(
                                Modifier()
                                    .setHeight(44_px),
                                TextboxOptions()
                                    .setPlaceholder("a textbox")
                            ),

                            gap(12_px),

                            // The widgets follow it too, inner parts included.
                            datefield(
                                Modifier()
                                    .setHeight(48_px),
                                DateFieldOptions()
                            ),

                            gap(12_px),

                            dropdown(
                                Modifier()
                                    .setHeight(40_px),
                                DropdownOptions()
                                    .setPlaceholder("a dropdown"),
                                {"one", "two", "three"}
                            )
                        }
                    ),

                    spacer(
                        Modifier()
                            .setWidth(32_px)
                    ),

                    // ---- Rows that opt out ----
                    column(
                        Modifier()
                            .setWidth(300_px),
                        ColumnOptions(),
                        contains {
                            caption("Overrides it"),

                            overrideRow(
                                "setRounding(0) -- stays square",
                                RowOptions()
                                    .setColorRole("panel")
                                    .setRounding(0.f)
                            ),

                            gap(12_px),

                            overrideRow(
                                "setRounding(28) -- stays round",
                                RowOptions()
                                    .setColorRole("panel")
                                    .setRounding(28.f)
                            ),

                            gap(12_px),

                            // Borders are per-element only, never themed.
                            overrideRow(
                                "1px outline, set by hand",
                                RowOptions()
                                    .setColorRole("panel")
                                    .setOutlineColorRole("outline")
                                    .setOutlineThickness(1.f)
                            ),

                            gap(12_px),

                            overrideRow(
                                "3px accent border",
                                RowOptions()
                                    .setColorRole("panel")
                                    .setOutlineColorRole("accent")
                                    .setOutlineThickness(3.f)
                            ),

                            gap(12_px),

                            // A literal colour wins too: no role, so no palette.
                            overrideRow(
                                "literal colour -- ignores palette",
                                RowOptions()
                                    .setColor({120, 60, 60, 255})
                            )
                        }
                    )
                }
            ),

            gap(100_pct)
        },
        "root"
    );
}
