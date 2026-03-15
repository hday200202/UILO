#define UILO_RAYLIB
#include "../UILO.hpp"

int main() {
    uilo::RaylibApp app("UILO Raylib Test", 1920, 1080);
    app.setScale(2.f);

    // --- Top bar: title + buttons ---
    auto* topBar = uilo::row(
        uilo::Modifier()
            .setFixedHeight(44)
            .setColor(uilo::Color(40, 40, 40)),
        uilo::contains{
            uilo::spacer(uilo::Modifier().setFixedWidth(12)),

            uilo::text(
                uilo::Modifier()
                .setColor(uilo::Color::White())
                .align(uilo::Align::CENTER_Y), 
                "UILO Raylib Test"
            ),

            uilo::spacer(),

            uilo::button(
                uilo::Modifier()
                    .setFixedWidth(80)
                    .setFixedHeight(30)
                    .setColor(uilo::Color(70, 130, 70))
                    .align(uilo::Align::CENTER_Y)
                    .onLClick([] { std::cout << "[Save]\n"; }),
                uilo::ButtonStyle::Pill, 
                "Save"
            ),

            uilo::spacer(uilo::Modifier().setFixedWidth(8)),

            uilo::button(
                uilo::Modifier()
                    .setFixedWidth(80)
                    .setFixedHeight(30)
                    .setColor(uilo::Color(130, 70, 70))
                    .align(uilo::Align::CENTER_Y)
                    .onLClick([] { std::cout << "[Quit]\n"; }),
                uilo::ButtonStyle::Pill, "Quit"
            ),

            uilo::spacer(uilo::Modifier().setFixedWidth(12)),
        },
        "topBar"
    );

    // --- Sidebar with sliders and a knob ---
    auto* sidebar = uilo::column(
        uilo::Modifier()
            .setFixedWidth(200)
            .setColor(uilo::Color(30, 30, 30)),
        uilo::contains{
            uilo::text(
                uilo::Modifier()
                    .setColor(uilo::Color(180, 180, 180))
                    .setFixedHeight(24)
                    .align(uilo::Align::CENTER_X), 
                "Controls"
            ),

            uilo::spacer(uilo::Modifier().setFixedHeight(12)),

            uilo::text(
                uilo::Modifier()
                    .setColor(uilo::Color(140, 140, 140))
                    .setFixedHeight(18), 
                "Volume"
            ),

            uilo::horizontalSlider(
                uilo::Modifier()
                    .setFixedHeight(28)
                    .setColor(uilo::Color(50, 50, 50)),
                uilo::Color(100, 180, 255), 
                uilo::Color(60, 60, 60), 
                0.75f, 
                "volumeSlider"
            ),

            uilo::spacer(uilo::Modifier().setFixedHeight(12)),

            uilo::text(
                uilo::Modifier()
                    .setColor(uilo::Color(140, 140, 140))
                    .setFixedHeight(18), 
                "Pan"
            ),

            uilo::horizontalSlider(
                uilo::Modifier()
                    .setFixedHeight(28)
                    .setColor(uilo::Color(50, 50, 50)),
                uilo::Color(255, 180, 100), 
                uilo::Color(60, 60, 60), 
                0.5f, 
                "panSlider"
            ),

            uilo::spacer(uilo::Modifier().setFixedHeight(16)),

            uilo::text(
                uilo::Modifier()
                    .setColor(uilo::Color(140, 140, 140))
                    .setFixedHeight(18)
                    .align(uilo::Align::CENTER_X), 
                "Gain"
            ),

            uilo::knob(
                uilo::Modifier()
                    .setFixedWidth(80)
                    .setFixedHeight(80)
                    .setColor(uilo::Color(45, 45, 45))
                    .align(uilo::Align::CENTER_X),
                uilo::Color::White(), 
                uilo::Color(80, 80, 80), 
                uilo::Color(100, 200, 100), 
                0.5f, 
                "gainKnob"
            ),

            uilo::spacer(uilo::Modifier().setFixedHeight(16)),

            uilo::text(
                uilo::Modifier()
                    .setColor(uilo::Color(140, 140, 140))
                    .setFixedHeight(18), 
                "Preset"
            ),

            uilo::dropdown(
                uilo::Modifier()
                    .setFixedHeight(30)
                    .setColor(uilo::Color(55, 55, 55)),
                "Default",
                {"Default", "Warm", "Bright", "Dark"},
                "",
                uilo::Color::White(),
                uilo::Color(45, 45, 45),
                "presetDropdown"
            ),

            uilo::spacer(),

            uilo::textBox(
                uilo::Modifier()
                    .setFixedHeight(30)
                    .setColor(uilo::Color(60, 60, 60)),
                uilo::TBStyle::Pill,
                "",
                "Search...",
                uilo::Color::White(),
                uilo::Color(100, 180, 255),
                "searchBox"
            ),
            uilo::spacer(uilo::Modifier().setFixedHeight(8)),
        },
        "sidebar"
    );

    // --- Main content area with a grid of items ---
    std::initializer_list<uilo::Element*> gridItems = {
        uilo::button(uilo::Modifier().setColor(uilo::Color(60, 80, 120)).setWidth(1).setHeight(1), uilo::ButtonStyle::Rect, "A"),
        uilo::button(uilo::Modifier().setColor(uilo::Color(80, 60, 120)).setWidth(1).setHeight(1), uilo::ButtonStyle::Rect, "B"),
        uilo::button(uilo::Modifier().setColor(uilo::Color(120, 60, 80)).setWidth(1).setHeight(1), uilo::ButtonStyle::Rect, "C"),
        uilo::button(uilo::Modifier().setColor(uilo::Color(60, 120, 80)).setWidth(1).setHeight(1), uilo::ButtonStyle::Rect, "D"),
        uilo::button(uilo::Modifier().setColor(uilo::Color(120, 100, 60)).setWidth(1).setHeight(1), uilo::ButtonStyle::Rect, "E"),
        uilo::button(uilo::Modifier().setColor(uilo::Color(60, 100, 120)).setWidth(1).setHeight(1), uilo::ButtonStyle::Rect, "F"),
        uilo::button(uilo::Modifier().setColor(uilo::Color(100, 60, 100)).setWidth(1).setHeight(1), uilo::ButtonStyle::Rect, "G"),
        uilo::button(uilo::Modifier().setColor(uilo::Color(80, 120, 60)).setWidth(1).setHeight(1), uilo::ButtonStyle::Rect, "H"),
        uilo::button(uilo::Modifier().setColor(uilo::Color(100, 80, 60)).setWidth(1).setHeight(1), uilo::ButtonStyle::Rect, "I"),
    };

    auto* contentGrid = uilo::grid(
        uilo::Modifier()
            .setColor(uilo::Color(20, 20, 20)),
        120.f, 120.f, 3, 0,
        gridItems,
        "contentGrid"
    );

    auto* contentArea = uilo::column(
        uilo::Modifier()
            .setColor(uilo::Color(22, 22, 22)),
        uilo::contains{
            uilo::row(
                uilo::Modifier()
                    .setFixedHeight(32)
                    .setColor(uilo::Color(35, 35, 35)), 
                uilo::contains {
                    uilo::spacer(uilo::Modifier().setFixedWidth(12)),
                    uilo::text(
                        uilo::Modifier()
                            .setColor(uilo::Color(160, 160, 160))
                            .align(uilo::Align::CENTER_Y), 
                        "Items"
                    ),
                    uilo::spacer(),
                }
            ),
            contentGrid,
        },
        "contentArea"
    );

    // --- Status bar ---
    auto* statusBar = uilo::row(
        uilo::Modifier()
            .setFixedHeight(24)
            .setColor(uilo::Color(35, 35, 35)),
        uilo::contains{
            uilo::spacer(uilo::Modifier().setFixedWidth(8)),
            uilo::text(
                uilo::Modifier()
                    .setColor(uilo::Color(120, 120, 120))
                    .align(uilo::Align::CENTER_Y), 
                "Ready"
            ),
            uilo::spacer(),
            uilo::text(
                uilo::Modifier()
                    .setColor(uilo::Color(100, 100, 100))
                    .align(uilo::Align::CENTER_Y | uilo::Align::RIGHT), 
                "v1.0"
            ),
            uilo::spacer(uilo::Modifier().setFixedWidth(8)),
        },
        "statusBar"
    );

    // --- Root layout ---
    auto* body = uilo::row(uilo::Modifier(), {sidebar, contentArea});

    auto* root = uilo::column(
        uilo::Modifier().setColor(uilo::Color(18, 18, 18)),
        { topBar, body, statusBar }
    );

    app.addPage(uilo::page({root}), "main");
    app.setFullClean(true);

    while (app.isRunning()) {
        app.update();
        app.render();
    }

    return 0;
}
