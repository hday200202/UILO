#define UILO_SFML
#include "../UILO.hpp"

int main() {
    using namespace uilo;

    SFMLApp app("UILO sfml Test", 1920, 1080);
    app.setScale(2.f);

    // --- Top bar: title + buttons ---
    auto* topBar = row(
        Modifier()
            .setFixedHeight(44)
            .setColor(Color(40, 40, 40)),
        contains{
            spacer(Modifier().setFixedWidth(12)),

            text(
                Modifier()
                .setColor(Color::White())
                .align(Align::CENTER_Y), 
                "UILO Test"
            ),

            spacer(),

            button(
                Modifier()
                    .setFixedWidth(80)
                    .setFixedHeight(30)
                    .setColor(Color(70, 130, 70))
                    .align(Align::CENTER_Y)
                    .onLClick([] { std::cout << "[Save]\n"; }),
                ButtonStyle::Pill, 
                "Save"
            ),

            spacer(Modifier().setFixedWidth(8)),

            button(
                Modifier()
                    .setFixedWidth(80)
                    .setFixedHeight(30)
                    .setColor(Color(130, 70, 70))
                    .align(Align::CENTER_Y)
                    .onLClick([] { std::cout << "[Quit]\n"; }),
                ButtonStyle::Pill, "Quit"
            ),

            spacer(Modifier().setFixedWidth(12)),
        },
        "topBar"
    );

    // --- Sidebar with sliders and a knob ---
    auto* sidebar = column(
        Modifier()
            .setFixedWidth(200)
            .setColor(Color(30, 30, 30)),
        contains{
            text(
                Modifier()
                    .setColor(Color(180, 180, 180))
                    .setFixedHeight(24)
                    .align(Align::CENTER_X), 
                "Controls"
            ),

            spacer(Modifier().setFixedHeight(12)),

            text(
                Modifier()
                    .setColor(Color(140, 140, 140))
                    .setFixedHeight(18), 
                "Volume"
            ),

            horizontalSlider(
                Modifier()
                    .setFixedHeight(28)
                    .setColor(Color(50, 50, 50)),
                Color(100, 180, 255), 
                Color(60, 60, 60), 
                0.75f, 
                "volumeSlider"
            ),

            spacer(Modifier().setFixedHeight(12)),

            text(
                Modifier()
                    .setColor(Color(140, 140, 140))
                    .setFixedHeight(18), 
                "Pan"
            ),

            horizontalSlider(
                Modifier()
                    .setFixedHeight(28)
                    .setColor(Color(50, 50, 50)),
                Color(255, 180, 100), 
                Color(60, 60, 60), 
                0.5f, 
                "panSlider"
            ),

            spacer(Modifier().setFixedHeight(16)),

            text(
                Modifier()
                    .setColor(Color(140, 140, 140))
                    .setFixedHeight(18)
                    .align(Align::CENTER_X), 
                "Gain"
            ),

            knob(
                Modifier()
                    .setFixedWidth(80)
                    .setFixedHeight(80)
                    .setColor(Color(45, 45, 45))
                    .align(Align::CENTER_X),
                Color::White(), 
                Color(80, 80, 80), 
                Color(100, 200, 100), 
                0.5f, 
                "gainKnob"
            ),

            spacer(Modifier().setFixedHeight(16)),

            text(
                Modifier()
                    .setColor(Color(140, 140, 140))
                    .setFixedHeight(18), 
                "Preset"
            ),

            dropdown(
                Modifier()
                    .setFixedHeight(30)
                    .setColor(Color(55, 55, 55)),
                "Default",
                {"Default", "Warm", "Bright", "Dark"},
                "",
                Color::White(),
                Color(45, 45, 45),
                "presetDropdown"
            ),

            spacer(), // push remaining to bottom

            textBox(
                Modifier()
                    .setFixedHeight(30)
                    .setColor(Color(60, 60, 60)),
                TBStyle::Pill,
                "",
                "Search...",
                Color::White(),
                Color(100, 180, 255),
                "searchBox"
            ),
            spacer(Modifier().setFixedHeight(8)),
        },
        "sidebar"
    );

    // --- Main content area with a grid of items ---
    std::initializer_list<Element*> gridItems = {
        button(Modifier().setColor(Color(60, 80, 120)).setWidth(1).setHeight(1), ButtonStyle::Rect, "A"),
        button(Modifier().setColor(Color(80, 60, 120)).setWidth(1).setHeight(1), ButtonStyle::Rect, "B"),
        button(Modifier().setColor(Color(120, 60, 80)).setWidth(1).setHeight(1), ButtonStyle::Rect, "C"),
        button(Modifier().setColor(Color(60, 120, 80)).setWidth(1).setHeight(1), ButtonStyle::Rect, "D"),
        button(Modifier().setColor(Color(120, 100, 60)).setWidth(1).setHeight(1), ButtonStyle::Rect, "E"),
        button(Modifier().setColor(Color(60, 100, 120)).setWidth(1).setHeight(1), ButtonStyle::Rect, "F"),
        button(Modifier().setColor(Color(100, 60, 100)).setWidth(1).setHeight(1), ButtonStyle::Rect, "G"),
        button(Modifier().setColor(Color(80, 120, 60)).setWidth(1).setHeight(1), ButtonStyle::Rect, "H"),
        button(Modifier().setColor(Color(100, 80, 60)).setWidth(1).setHeight(1), ButtonStyle::Rect, "I"),
    };

    auto* contentGrid = grid(
        Modifier()
            .setColor(Color(20, 20, 20)),
        120.f, 120.f, 3, 0,
        gridItems,
        "contentGrid"
    );

    auto* contentArea = column(
        Modifier()
            .setColor(Color(22, 22, 22)),
        contains{
            row(
                Modifier()
                    .setFixedHeight(32)
                    .setColor(Color(35, 35, 35)), 
                contains {
                    spacer(Modifier().setFixedWidth(12)),
                    text(
                        Modifier()
                            .setColor(Color(160, 160, 160))
                            .align(Align::CENTER_Y), 
                        "Items"
                    ),
                    spacer(),
                }
            ),
            contentGrid,
        },
        "contentArea"
    );

    // --- Status bar ---
    auto* statusBar = row(
        Modifier()
            .setFixedHeight(24)
            .setColor(Color(35, 35, 35)),
        contains{
            spacer(Modifier().setFixedWidth(8)),
            text(
                Modifier()
                    .setColor(Color(120, 120, 120))
                    .align(Align::CENTER_Y), 
                "Ready"
            ),
            spacer(),
            text(
                Modifier()
                    .setColor(Color(100, 100, 100))
                    .align(Align::CENTER_Y | Align::RIGHT), 
                "v1.0"
            ),
            spacer(Modifier().setFixedWidth(8)),
        },
        "statusBar"
    );

    // --- Root layout ---
    auto* body = row(Modifier(), {sidebar, contentArea});

    auto* root = column(
        Modifier().setColor(Color(18, 18, 18)),
        { topBar, body, statusBar }
    );

    app.addPage(page({root}), "main");
    app.setFullClean(true);

    while (app.isRunning()) {
        app.update();
        app.render();
    }

    return 0;
}
