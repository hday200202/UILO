// Web: the same tree served to a browser instead of a window.
//
// There is no renderer, no event loop and no frame here. Everything above
// runWeb() is the code a desktop example would write -- pages, palette,
// callbacks -- and runWeb() serves it and blocks until the server stops.
//
// This uses the per-session form, UILO::runWeb(factory, config): the factory
// runs once per browser connection, on that connection's thread, and the
// WebApp it returns is destroyed with the connection. So every visitor gets
// their own UILO, their own counter and their own palette, and no two sessions
// touch the same tree. The single-instance form is one line --
//
//     ui.runWeb(config);
//
// -- and gives every connection a view of the SAME UILO, which is what a
// single-user local tool wants and what a multi-user one must not have: Wt
// drives connections from different threads with no lock between them.
//
// Build and run (from the repo root):
//
//     ./build.sh wt deps      # once: clones Wt + Boost into ext/ and builds them
//     ./build.sh wt           # builds UILO and the examples for the web
//     examples/bin/webapp     # then open http://localhost:8080
//
// A normal desktop build still compiles this file -- it just produces a binary
// that tells you to rebuild with `wt`, so the glob in examples/CMakeLists.txt
// needs no special case.

#include "../include/UILO.hpp"

#ifndef UILO_WT

#include <cstdio>

int main() {
    std::puts(
        "webapp: this example needs the web backend.\n"
        "  ./build.sh wt deps   # once, fetches and builds ext/wt + ext/boost\n"
        "  ./build.sh wt        # then run examples/bin/webapp"
    );
    return 1;
}

#else

#include <chrono>
#include <ctime>
#include <memory>
#include <string>

using namespace uilo;

static Palette sessionDark() {
    Palette p;
    p.set("app.bg",   { 24,  26,  35, 255});
    p.set("panel",    { 36,  39,  51, 255});
    p.set("panelAlt", { 48,  52,  67, 255});
    p.set("text",     {198, 206, 232, 255});
    p.set("textDim",  {132, 141, 172, 255});
    p.set("accent",   {126, 160, 240, 255});
    p.set("onAccent", {255, 255, 255, 255});
    p.set("outline",  { 62,  67,  84, 255});
    return p;
}

static Palette sessionLight() {
    Palette p;
    p.set("app.bg",   {238, 241, 247, 255});
    p.set("panel",    {252, 253, 255, 255});
    p.set("panelAlt", {224, 229, 240, 255});
    p.set("text",     { 34,  40,  62, 255});
    p.set("textDim",  { 98, 107, 134, 255});
    p.set("accent",   { 58, 106, 214, 255});
    p.set("onAccent", {255, 255, 255, 255});
    p.set("outline",  {203, 210, 224, 255});
    return p;
}


/*
    sectionHeading(const std::string& content, unsigned size):
    - Params:   const std::string& content, unsigned size
    - Returns:  Text*
    - Desc:     A left-aligned line of text at a given size, following the
                palette rather than naming a colour.
*/
static Text* sectionHeading(const std::string& content, unsigned size) {
    return text(
        Modifier()
            .setHeight(Dimension{(float)size + 10.f, false}),
        TextOptions()
            .setContent(content)
            .setCharSize(size)
            .setColorRole("text")
            .setTextAlignX(Align::Left)
            .setTextAlignY(Align::CenterY)
    );
}


/*
    label(const std::string& content):
    - Params:   const std::string& content
    - Returns:  Text*
    - Desc:     A dimmed caption line, for the text above a control.
*/
static Text* label(const std::string& content) {
    return text(
        Modifier()
            .setHeight(24_px),
        TextOptions()
            .setContent(content)
            .setCharSize(14)
            .setColorRole("textDim")
            .setTextAlignX(Align::Left)
            .setTextAlignY(Align::CenterY)
    );
}


/*
    Session:
    - Desc:     One browser connection's application. It owns the UILO, the
                elements that have to be reached from a callback, and the state
                those callbacks mutate -- so two visitors clicking at the same
                time are incrementing two different counters.
    - The factory below hands one of these to runWeb per connection, and the
      server destroys it when that connection ends. Nothing here is shared, and
      nothing here needs a lock.
*/
class Session : public wt::WebApp {
public:
    Session() {
        m_ui.getTheme().setPalette(sessionDark());
        m_ui.addPage(page(buildRoot(), "main"));
        m_ui.setPage("main");

        /* The web stand-in for work a desktop app would do in its update loop.
           Runs on this session's thread and re-syncs the tree afterwards, so
           the browser sees the new text without a click. */
        wt::every(std::chrono::seconds(1), [this] { refreshClock(); });
    }

    UILO& ui() override { return m_ui; }

private:
    Container* buildRoot() {
        return column(
            Modifier(),
            ColumnOptions()
                .setColorRole("app.bg")
                .setInnerPadding(24.f),
            contains{
                sectionHeading("UILO on the web", 30),
                label("The same element tree a desktop build would draw, "
                      "translated to HTML and CSS."),

                spacer(Modifier().setHeight(20_px)),

                card(contains{
                    label("Per-session state"),
                    m_countText = text(
                        Modifier().setHeight(34_px),
                        TextOptions()
                            .setContent("Clicked 0 times")
                            .setCharSize(24)
                            .setColorRole("text")
                            .setTextAlignX(Align::Left)
                            .setTextAlignY(Align::CenterY)
                    ),
                    label("Open a second browser: it gets its own count."),

                    spacer(Modifier().setHeight(12_px)),

                    row(
                        Modifier().setHeight(40_px),
                        RowOptions(),
                        contains{
                            actionButton("Count up",  "accent",   [this] { bump(+1); }),
                            spacer(Modifier().setWidth(10_px)),
                            actionButton("Count down", "panelAlt", [this] { bump(-1); }),
                            spacer(Modifier().setWidth(10_px)),
                            actionButton("Reset",      "panelAlt", [this] { setCount(0); }),
                            /* Eats the rest of the row so the buttons stay their
                               own width instead of stretching across it. */
                            spacer(Modifier().setWidth(1_flex)),
                        }
                    ),
                }),

                spacer(Modifier().setHeight(16_px)),

                card(contains{
                    label("Theme, per session"),
                    /* Each session owns its UILO, and each UILO owns its
                       theme -- so this repaints one browser and no other. */
                    dropdown(
                        Modifier().setHeight(38_px).setWidth(220_px),
                        DropdownOptions()
                            .setOnItemChanged([this](const std::string& choice) {
                                m_ui.getTheme().setPalette(
                                    choice == "Light" ? sessionLight() : sessionDark());
                            }),
                        {"Dark", "Light"},
                        "theme_select"
                    ),
                }),

                spacer(Modifier().setHeight(16_px)),

                card(contains{
                    label("Server clock, pushed from uilo::wt::every"),
                    m_clockText = text(
                        Modifier().setHeight(30_px),
                        TextOptions()
                            .setContent("--:--:--")
                            .setCharSize(22)
                            .setColorRole("accent")
                            .setTextAlignX(Align::Left)
                            .setTextAlignY(Align::CenterY)
                    ),
                }),

                /* Holds the content at the top: the shares divide what the
                   sized children leave, and this one is the only share. */
                spacer(Modifier().setHeight(1_flex)),
            },
            "root"
        );
    }

    /*
        card(contains children):
        - Params:   contains children
        - Returns:  Column*
        - Desc:     A panel that is only as tall as what it holds, by adding up
                    its children rather than filling its parent. Only pixel
                    heights are counted: a percentage or a share is a fraction
                    of the box being measured, so including one would be
                    circular.
    */
    Column* card(contains children) {
        float height = 0.f;
        for (Element* child : children) {
            const Dimension h = child->getModifier().getHeight();
            if (h.isAbsolute()) height += h.value;
        }

        return column(
            Modifier().setHeight(Dimension{height + 32.f, false}),
            ColumnOptions()
                .setColorRole("panel")
                .setInnerPadding(16.f)
                .setRounding(10.f)
                .setOutlineColorRole("outline")
                .setOutlineThickness(1.f),
            children
        );
    }

    /*
        actionButton(...):
        - Params:   const std::string& caption, const std::string& role,
                    std::function<void()> action
        - Returns:  Button*
        - Desc:     A fixed-width button whose fill follows a palette role.
    */
    Button* actionButton(
        const std::string& caption,
        const std::string& role,
        std::function<void()> action
    ) {
        return button(
            Modifier()
                .setWidth(130_px)
                .setOnLeftClick([action = std::move(action)](Element*) { action(); }),
            ButtonOptions()
                .setColorRole(role)
                .setRounding(8.f)
                .setLabel(text(
                    Modifier(),
                    TextOptions()
                        .setContent(caption)
                        .setCharSize(15)
                        .setColorRole(role == "accent" ? "onAccent" : "text")
                        .setTextAlignX(Align::CenterX)
                        .setTextAlignY(Align::CenterY)
                ))
        );
    }

    void bump(int by)     { setCount(m_count + by); }

    void setCount(int to) {
        m_count = to;
        m_countText->setString(
            "Clicked " + std::to_string(m_count) +
            (m_count == 1 || m_count == -1 ? " time" : " times"));
    }

    void refreshClock() {
        const std::time_t now = std::time(nullptr);
        std::tm tm{};
#ifdef _WIN32
        localtime_s(&tm, &now);
#else
        localtime_r(&now, &tm);
#endif
        char buf[16];
        std::strftime(buf, sizeof buf, "%H:%M:%S", &tm);
        m_clockText->setString(buf);
    }

    UILO  m_ui;
    Text* m_countText = nullptr;
    Text* m_clockText = nullptr;
    int   m_count     = 0;
};


int main() {
    wt::WebConfig config;
    config.title = "UILO web example";
    config.port  = 8080;
    /* Wt serves its own JS and themes out of this directory. The default is
       written for a project that vendors UILO under ext/UILO; in this repo Wt
       is cloned directly into ext/. */
    config.resourcesDir = "ext/wt/resources";
    /* Static files, served at /. Nothing here needs any, but Wt wants the
       directory to exist. */
    config.docRoot = ".";

    std::printf("[webapp] http://localhost:%d\n", config.port);
    return UILO::runWeb([] { return std::make_unique<Session>(); }, config);
}

#endif   /* UILO_WT */
