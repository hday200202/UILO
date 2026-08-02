#include <UILO.hpp>

using namespace uilo;


/*
    Forward Declarations:
    - My prefered way of building UIs in UILO
*/
Palette     buildPalette();
Container*  buildTitleBar();
Container*  buildTerminalPanel();
Container*  buildStatusBar();
Button*     barButton(const std::string& label, std::function<void()> onClick);
Page*       buildMainPage();

Terminal*   g_term   = nullptr;
Text*       g_status = nullptr;


/*
    main():
    - Initialize your renderer          (width, height, window title)
    - Initialize your UILO instance     (renderer, main page)
    - Initialize and set your palette
    - Set the render scale to the OS render scale
*/
int main() {
    Renderer renderer;
    renderer.init(1000, 640, "Terminal");

    UILO uilo(renderer, buildMainPage());

    uilo.setPalette(buildPalette());
    uilo.setScale(OS::scale());

    while (uilo.isRunning()) {
        // updates
        uilo.pollEvents();
        uilo.update();

        // rendering
        renderer.beginFrame();
        renderer.clear();

        uilo.render();

        renderer.endFrame();
    }

    return 0;
}


/*
    buildPalette():
    - Setting a few reusable color roles, and some global theme customizations
*/
Palette buildPalette() {
    Palette palette;

    palette.set("background", Color::fromHex("#1b1b1f"));
    palette.set("panel",      Color::fromHex("#242429"));
    palette.set("terminalBg", Color::fromHex("#16161a"));
    palette.set("terminalFg", Color::fromHex("#d6d6dd"));
    palette.set("accent",     Color::fromHex("#7ad0a0"));
    palette.set("dim",        Color::fromHex("#8a8a95"));
    palette.set("onAccent",   Color::fromHex("#16161a"));

    Theme::current().setRounding(8.f);

    return palette;
}


/*
    barButton(const std::string& label, std::function<void()> onClick):
    - A small labelled button for the title bar
*/
Button* barButton(const std::string& label, std::function<void()> onClick) {
    return button(
        Modifier()
            .setWidth(96_px)
            .setHeight(26_px)
            .setAlign(Align::CenterY)
            .setCursor(CursorType::Hand)
            .setOnLeftClick([onClick](Element*) { onClick(); }),
        ButtonOptions()
            .setColorRole("accent")
            .setRounding(6.f)
            .setLabel(text(
                Modifier()
                    .setAlign(Align::CenterX | Align::CenterY),
                TextOptions()
                    .setContent(label)
                    .setFont("assets/fonts/Montserrat.ttf")
                    .setCharSize(13)
                    .setColorRole("onAccent")
                    .setTextAlignX(Align::CenterX)
                    .setTextAlignY(Align::CenterY)
            ))
    );
}


/*
    buildTitleBar():
    - A label, and the two controls the terminal exposes: a command sent as
      though it had been typed, and a restart
*/
Container* buildTitleBar() {
    return row(
        Modifier()
            .setWidth(100_pct)
            .setHeight(44_px),
        RowOptions()
            .setColorRole("panel"),
        contains{
            spacer(Modifier().setWidth(14_px)),

            text(
                Modifier()
                    .setWidth(120_px)
                    .setHeight(100_pct)
                    .setAlign(Align::Left | Align::CenterY),
                TextOptions()
                    .setContent("Terminal")
                    .setFont("assets/fonts/Montserrat.ttf")
                    .setCharSize(15)
                    .setTextAlignX(Align::Left)
                    .setTextAlignY(Align::CenterY)
            ),

            spacer(Modifier().setWidth(100_pct)),

            barButton("ls -la", [] {
                if (g_term) g_term->send("ls -la\n");
            }),

            spacer(Modifier().setWidth(8_px)),

            barButton("restart", [] {
                if (!g_term) return;
                g_term->stop();
                g_term->start();
                if (g_status) g_status->setString("restarted");
            }),

            spacer(Modifier().setWidth(14_px)),
        }
    );
}


/*
    buildTerminalPanel():
    - The terminal itself. It fills whatever space it is given and re-measures
      its grid against the font, so resizing the window resizes the shell.
*/
Container* buildTerminalPanel() {
    g_term = terminal(
        Modifier()
            .setWidth(100_pct)
            .setHeight(100_pct),
        TerminalOptions()
            .setFont("assets/fonts/AdwaitaMonoNerdFont.ttf")
            .setCharSize(14)
            .setLineSpacing(1.15f)
            .setBackgroundColorRole("terminalBg")
            .setForegroundColorRole("terminalFg")
            .setCursorColorRole("accent")
            // .setPadding(10.f)
            .setRounding(8.f)
            .setScrollback(5000)
            .setOnExit([] {
                if (g_status) g_status->setString("shell exited -- press restart");
            }),
        "terminal"
    );

    return row(
        Modifier()
            .setWidth(100_pct)
            .setHeight(100_pct),
        RowOptions(),
        contains{
            spacer(Modifier().setWidth(10_px)),
            g_term,
            spacer(Modifier().setWidth(10_px)),
        }
    );
}


/*
    buildStatusBar():
    - A single line of text along the bottom, updated by the terminal callbacks
*/
Container* buildStatusBar() {
    g_status = text(
        Modifier()
            .setWidth(100_pct)
            .setHeight(100_pct)
            .setAlign(Align::Left | Align::CenterY),
        TextOptions()
            .setContent("click to focus  ·  drag to select  ·  Cmd+C / Cmd+V")
            .setFont("assets/fonts/Montserrat.ttf")
            .setCharSize(12)
            .setColorRole("dim")
            .setTextAlignX(Align::Left)
            .setTextAlignY(Align::CenterY)
    );

    return row(
        Modifier()
            .setWidth(100_pct)
            .setHeight(30_px),
        RowOptions(),
        contains{
            spacer(Modifier().setWidth(14_px)),
            g_status,
        }
    );
}


/*
    buildMainPage():
    - Title bar, terminal, and a status line along the bottom
*/
Page* buildMainPage() {
    return page(
        column(
            Modifier()
                .setWidth(100_pct)
                .setHeight(100_pct),
            ColumnOptions()
                .setColorRole("background"),
            contains{
                buildTitleBar(),
                spacer(Modifier().setHeight(10_px)),
                buildTerminalPanel(),
                buildStatusBar(),
            }
        ),
        "main"
    );
}
