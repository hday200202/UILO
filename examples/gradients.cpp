// Gradient showcase: literal gradients, palette-role stops, named palette
// gradients, rounded corners, and live mutation from a hover callback.
#include "../include/UILO.hpp"
#include "../include/renderer/Renderer.hpp"
#include <SDL3/SDL.h>
#include <cstdio>

using namespace uilo;

static Palette makePalette() {
    Palette p = Palette::defaultDark();
    p.set("app.bg", {24, 25, 34, 255});
    // Named gradient: elements reference it with setGradientRole("hero"),
    // so swapping the palette restyles them all at once.
    p.setGradient("hero",
        Gradient().setTop(Color{97, 62, 180}).setBottom(Color{34, 27, 58}));
    return p;
}

static Container* buildRoot() {
    return column(
        Modifier().setOuterPadding(16.f),
        ColumnOptions().setColorRole("app.bg"),
        contains{
            // Vertical fade, rounded. Position-named setters read top->bottom.
            row(
                Modifier().setHeight(22_pct).setOuterPadding(8.f),
                RowOptions()
                    .setGradient(Gradient()
                        .setTop(Color{240, 120, 90})
                        .setBottom(Color{140, 40, 80}))
                    .setRounding(18.f)
            ),

            // Horizontal gradient built from palette roles: follows theme.
            row(
                Modifier().setHeight(22_pct).setOuterPadding(8.f),
                RowOptions()
                    .setGradient(Gradient()
                        .setLeft("accent")
                        .setRight("panel"))
                    .setRounding(18.f)
            ),

            // Four explicit corners, each named by where it sits.
            row(
                Modifier().setHeight(22_pct).setOuterPadding(8.f),
                RowOptions()
                    .setGradient(Gradient()
                        .setTopLeft(Color{80, 170, 255})
                        .setTopRight(Color{170, 80, 255})
                        .setBottomLeft(Color{20, 60, 90})
                        .setBottomRight(Color{90, 20, 60}))
                    .setRounding(18.f)
            ),

            // Named palette gradient + a gradient button that swaps its
            // gradient from a hover callback.
            row(
                Modifier().setHeight(22_pct).setOuterPadding(8.f),
                RowOptions().setGradientRole("hero").setRounding(18.f),
                {
                    button(
                        Modifier()
                            .setWidth(40_pct).setHeight(60_pct)
                            .setAlign(Align::CenterX | Align::CenterY)
                            .setOnHoverEnter([](Button* b) {
                                b->getOptions().setGradient(Gradient()
                                    .setTop(Color{255, 210, 120})
                                    .setBottom(Color{210, 120, 60}));
                            })
                            .setOnHoverExit([](Button* b) {
                                b->getOptions().setGradient(Gradient()
                                    .setTop(Color{120, 210, 170})
                                    .setBottom(Color{40, 110, 90}));
                            }),
                        ButtonOptions()
                            .setGradient(Gradient()
                                .setTop(Color{120, 210, 170})
                                .setBottom(Color{40, 110, 90}))
                            .setRounding(14.f)
                    ),
                }
            ),
        }
    );
}

int main() {
    Renderer renderer;
    if (!renderer.init(1000, 680, "UILO Gradients", 8)) {
        std::fprintf(stderr, "Failed to initialize renderer\n");
        return 1;
    }

    UILO ui;
    ui.setRenderer(renderer);
    ui.setPalette(makePalette());
    ui.addPage(page(buildRoot(), "main"));
    ui.setPage("main");

    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) running = false;
            ui.handleEvent(event);
        }
        ui.update();

        renderer.beginFrame();
        renderer.clear(ui.getPalette().get("app.bg"));
        ui.render();
        renderer.endFrame();
    }
    return 0;
}
