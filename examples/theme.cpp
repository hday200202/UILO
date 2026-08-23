// Theme: where the look of an application lives.
//
// This file is split the way a web page is split. buildTheme() is the
// stylesheet: every colour, size, radius and spacing value in this example is
// inside that one function, grouped by what it styles. buildRoot() is the
// markup: it calls h1(), card(), primaryButton() and names roles, and says
// nothing about how any of them look.
//
// If you want to change how this example looks, buildTheme() is the only
// function you have to open.
//
// A theme belongs to a UILO -- ui.getTheme() / ui.setTheme() -- and is applied
// when an element binds to it, filling in whatever the call site left alone and
// never overriding a setting. That is what lets the buttons across the top
// restyle a running app: nothing here is rebuilt, on either row.
//
// The defaults all of this starts from are in include/Defaults.hpp.

#include "../include/UILO.hpp"
#include "../include/Defaults.hpp"
#include "../include/renderer/Renderer.hpp"
#include <SDL3/SDL.h>
#include <cstdio>

using namespace uilo;

namespace {

/* ========================================================================== */
/*  STYLESHEET                                                                */
/*                                                                            */
/*  Everything this example looks like is in this section and nowhere else.    */
/*  Nothing below it names a colour, a character size, a radius or a gap.      */
/* ========================================================================== */

/*
    applyShape(Theme& theme, float radius):
    - Params:   Theme& theme, float radius
    - Returns:  void
    - Desc:     The corner radius of every surface, in one place. This is all
                22 rounding knobs the library has -- one line per surface a type
                draws, which is the trade for those types being able to carry a
                whole look rather than one shared number. Miss one and that
                surface quietly keeps its old shape, which is exactly what a
                single global setting could never get wrong.
    - The page and pane roles are left out on purpose: they are backgrounds that
      run to the window edge, and rounding those would round the app itself.
*/
void applyShape(Theme& theme, float radius) {
    /* Surfaces. */
    theme.edit<ColumnOptions>().setRounding(radius);
    theme.edit<RowOptions>().setRounding(radius);
    theme.edit<CanvasOptions>().setRounding(radius);
    theme.edit<SpacerOptions>().setRounding(radius);
    theme.edit<WaveformOptions>().setRounding(radius);

    /* Controls. */
    theme.edit<ButtonOptions>().setRounding(radius);
    theme.edit<TextboxOptions>().setRounding(radius);

    SliderOptions& slider = theme.edit<SliderOptions>();
    slider.setTrackRounding(radius);
    slider.setThumbRounding(radius);

    DropdownOptions& dropdown = theme.edit<DropdownOptions>();
    dropdown.setHeaderRounding(radius);
    dropdown.setPopupRounding(radius);
    dropdown.setItemRounding(radius);

    /* Composite widgets, each of which rounds more than one surface. */
    theme.edit<ContextMenuOptions>().setRounding(radius);
    theme.edit<TerminalOptions>().setRounding(radius);
    theme.edit<DateFieldOptions>().setRounding(radius);

    DatePickerOptions& picker = theme.edit<DatePickerOptions>();
    picker.setRounding(radius);
    picker.setNavRounding(radius);
    picker.setCellRounding(radius);
    picker.setButtonRounding(radius);

    FileBrowserOptions& browser = theme.edit<FileBrowserOptions>();
    browser.setRounding(radius);
    browser.setEntryRounding(radius);
    browser.setHeaderRounding(radius);

    /* A role that pins its own radius has to be told separately -- that is what
       pinning means, and it is why "page" and "pane" below stay square. */
    theme.edit<ColumnOptions>("card").setRounding(radius);
}


/*
    buildTheme():
    - Params:   none
    - Returns:  Theme
    - Desc:     The whole look of this example. Starts from defaultTheme(), so
                the roles Defaults.hpp already declares -- h1, h2, h3, body,
                caption, label, primary, ghost, panel, card -- arrive for free
                and only what this app adds or changes is spelled out below.
*/
Theme buildTheme() {
    Theme theme = defaultTheme();

    /* ---- Shape ---------------------------------------------------------- */
    applyShape(theme, 8.f);

    /* ---- Spacing scale ---------------------------------------------------
       Vertical gaps for columns, horizontal gaps for rows. A spacer built with
       a plain Modifier() is a 1_flex push, so it needs no role. */
    theme.edit<Modifier>("gap.sm").setHeight(8_px);
    theme.edit<Modifier>("gap.md").setHeight(16_px);
    theme.edit<Modifier>("gap.lg").setHeight(24_px);

    theme.edit<Modifier>("hgap.sm").setWidth(8_px);
    theme.edit<Modifier>("hgap.md").setWidth(16_px);
    theme.edit<Modifier>("hgap.lg").setWidth(24_px);

    /* ---- Boxes -----------------------------------------------------------
       The size half of each role. h1/h2/h3/caption already carry theirs from
       Defaults.hpp. */
    theme.edit<Modifier>("bar").setHeight(36_px);
    theme.edit<Modifier>("body").setHeight(48_px);
    theme.edit<Modifier>("field").setHeight(44_px);
    theme.edit<Modifier>("pane").setWidth(320_px);

    Modifier& switchBox = theme.edit<Modifier>("switch");
    switchBox.setWidth(96_px);
    switchBox.setHeight(36_px);

    /* The button helpers take their box from the same role as their fill, so
       giving "primary" a width here is all a call site needs. */
    theme.edit<Modifier>("primary").setWidth(120_px);
    theme.edit<Modifier>("ghost").setWidth(120_px);

    /* ---- Surfaces --------------------------------------------------------
       The page background and the two panes stay square whatever the shape
       buttons do, since applyShape leaves these roles alone. */
    ColumnOptions& page = theme.edit<ColumnOptions>("page");
    page.setColorRole("app.bg");
    page.setInnerPadding(24.f);
    page.setRounding(0.f);

    theme.edit<ColumnOptions>("pane").setRounding(0.f);
    theme.edit<RowOptions>("bar").setRounding(0.f);

    theme.edit<ButtonOptions>("switch").setColorRole("panelAlt");

    return theme;
}


/* ========================================================================== */
/*  MARKUP                                                                    */
/*                                                                            */
/*  Structure and content only. Every look here is a role name defined above.  */
/* ========================================================================== */

/*
    switchButton(UILO& ui, const std::string& text, std::function<void(Theme&)> restyle):
    - Params:   UILO& ui, const std::string& text,
                std::function<void(Theme&)> restyle
    - Returns:  Button*
    - Desc:     One of the buttons across the top. Each edits the UILO's theme
                in place and then asks for it to be re-applied, which is what
                restyles the tree without rebuilding it.
    - The long form rather than primaryButton(), because these want a fill role
      of their own -- which is the point at which a helper stops being the
      shorter way to say something.
*/
Button* switchButton(
    UILO& ui,
    const std::string& text,
    std::function<void(Theme&)> restyle
) {
    return button(
        Modifier("switch")
            .setOnLeftClick([&ui, restyle](Element*) {
                restyle(ui.getTheme());
                ui.refreshTheme();
            }),
        ButtonOptions("switch")
            .setLabel(heading("label", text))
    );
}


/*
    buildRoot(UILO& ui):
    - Params:   UILO& ui
    - Returns:  Container*
    - Desc:     The whole tree, built once. The UILO is threaded through only so
                the buttons can reach the theme they restyle.
*/
Container* buildRoot(UILO& ui) {
    return column(
        Modifier(),
        ColumnOptions("page"),
        contains {
            h1("uilo::Theme"),
            caption("Every button here restyles the tree in place. Nothing is rebuilt."),

            spacer(Modifier("gap.lg")),

            row(
                Modifier("bar"),
                RowOptions("bar"),
                contains {
                    switchButton(ui, "Dark",  [](Theme& t) { t.setPalette(darkPalette()); }),
                    spacer(Modifier("hgap.sm")),
                    switchButton(ui, "Light", [](Theme& t) { t.setPalette(lightPalette()); }),
                    spacer(Modifier("hgap.lg")),
                    switchButton(ui, "Square",  [](Theme& t) { applyShape(t, 0.f); }),
                    spacer(Modifier("hgap.sm")),
                    switchButton(ui, "Rounded", [](Theme& t) { applyShape(t, 8.f); }),
                    spacer(Modifier("hgap.sm")),
                    switchButton(ui, "Pill",    [](Theme& t) { applyShape(t, 18.f); }),
                    spacer(Modifier())
                }
            ),

            spacer(Modifier("gap.lg")),

            row(
                Modifier(),
                RowOptions("bar"),
                contains {
                    /* ---- Built with empty options ---- */
                    column(
                        Modifier("pane"),
                        ColumnOptions("pane"),
                        contains {
                            caption("Follows the theme"),

                            spacer(Modifier("gap.sm")),

                            textbox(
                                Modifier("field"),
                                TextboxOptions()
                                    .setPlaceholder("a textbox")
                            ),

                            spacer(Modifier("gap.md")),

                            /* The composite widgets follow it too, inner parts
                               included. */
                            datefield(
                                Modifier("field"),
                                DateFieldOptions()
                            ),

                            spacer(Modifier("gap.md")),

                            dropdown(
                                Modifier("field"),
                                DropdownOptions()
                                    .setPlaceholder("a dropdown"),
                                {"one", "two", "three"}
                            ),

                            spacer(Modifier())
                        }
                    ),

                    spacer(Modifier("hgap.lg")),

                    /* ---- Built out of the named-role helpers ---- */
                    column(
                        Modifier(),
                        ColumnOptions("pane"),
                        contains {
                            caption("Named roles"),

                            spacer(Modifier("gap.sm")),

                            card(contains {
                                h2("Heading two"),
                                h3("Heading three"),

                                spacer(Modifier("gap.sm")),

                                body("Body copy, at the size the theme gives it. "
                                     "The call site says what this is, not what "
                                     "it looks like."),

                                spacer(Modifier("gap.md")),

                                row(
                                    Modifier("bar"),
                                    RowOptions("bar"),
                                    contains {
                                        primaryButton("Primary", [] {}),
                                        spacer(Modifier("hgap.md")),
                                        ghostButton("Ghost", [] {}),
                                        spacer(Modifier())
                                    }
                                ),

                                spacer(Modifier())
                            })
                        }
                    )
                }
            )
        },
        "root"
    );
}

} // namespace


int main() {
    Renderer renderer;
    if (!renderer.init(1100, 780, "Theme", 16)) {
        std::fprintf(stderr, "Failed to initialize renderer\n");
        return 1;
    }

    UILO ui;
    ui.setRenderer(renderer);
    ui.setScale(OS::scale());
    ui.setTheme(buildTheme());

    ui.addPage(page(buildRoot(ui), "main_page"));
    ui.setPage("main_page");

    while (ui.isRunning()) {
        ui.pollEvents();
        ui.update();

        renderer.beginFrame();
        /* Read through getPalette(), so it follows the theme live. */
        renderer.clear(ui.getPalette().get("app.bg"));
        ui.render();
        renderer.endFrame();
    }
    return 0;
}
