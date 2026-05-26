#include "../include/UILO.hpp"
#include "../include/renderer/Renderer.hpp"
#include <SDL3/SDL.h>
#include <iostream>
#include <cstdio>

using namespace uilo;

Color   BG_COLOR    = {33, 35, 47};
Color   CONT_COLOR  = {44, 47, 60};
float   ROUNDING    = 8.f;

Container* buildRootContainer();

int main() {
    Renderer renderer;
    if (!renderer.init(3840, 2160, "Containers", 0)) {
        std::fprintf(stderr, "Failed to initialize renderer\n");
        return 1;
    }

    renderer.setVsync(false);

    UILO ui(renderer, page(buildRootContainer(), "main_page"));
    ui.setScale(0.75f);

    Font fpsFont = renderer.loadFont("assets/fonts/Montserrat.ttf");

    bool prevPlus  = false;
    bool prevMinus = false;
    bool prevF10   = false;
    bool showFps   = false;
    bool running   = true;
    bool prevV     = false;

    float fpsTimer = 0.f;
    int   fpsLoops = 0;
    float fpsValue = 0.f;

    while (running) {
        const float dt = ui.getDeltaTime();
        fpsTimer += dt;
        fpsLoops++;
        if (fpsTimer >= 0.25f) {
            fpsValue = (float)fpsLoops / fpsTimer;
            fpsLoops = 0;
            fpsTimer = 0.f;
        }

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT)
                running = false;
            if (event.type == SDL_EVENT_KEY_DOWN) {
                SDL_Keycode k = event.key.key;
                if (k == SDLK_EQUALS || k == SDLK_KP_PLUS) {
                    bool prevP = prevPlus;
                    prevPlus = true;
                    if (!prevP) ui.setScale(ui.getScale() + 0.1f);
                } else if (k == SDLK_MINUS || k == SDLK_KP_MINUS) {
                    bool prevM = prevMinus;
                    prevMinus = true;
                    if (!prevM) ui.setScale(ui.getScale() - 0.1f);
                } else if (k == SDLK_F10) {
                    if (!prevF10) showFps = !showFps;
                    prevF10 = true;
                } else if (k == SDLK_V) {
                    if (!prevV) renderer.setVsync(!renderer.getVsync());
                    prevV = true;
                }
            }
            if (event.type == SDL_EVENT_KEY_UP) {
                SDL_Keycode k = event.key.key;
                if (k == SDLK_EQUALS || k == SDLK_KP_PLUS)  prevPlus  = false;
                if (k == SDLK_MINUS  || k == SDLK_KP_MINUS) prevMinus = false;
                if (k == SDLK_F10)                          prevF10   = false;
            }
            ui.handleEvent(event);
        }

        ui.update();

        renderer.beginFrame();
        renderer.clear(BG_COLOR);
        ui.render();

        if (showFps && fpsFont.valid()) {
            char buf[32];
            std::snprintf(buf, sizeof(buf), "%.0f FPS", fpsValue);
            const float scale = ui.getScale();
            const float pxH   = 18.f * scale;
            const float pad   = 8.f  * scale;
            renderer.drawText(buf, { pad, pad }, fpsFont, pxH, {255, 255, 255});
        }

        renderer.endFrame();
    }
}

Container* buildRootContainer() {
    auto panelCol = ColumnOptions().setColor(CONT_COLOR).setRounding(ROUNDING);
    auto panelRow = RowOptions().setColor(CONT_COLOR).setRounding(ROUNDING);
    
    return column(
        Modifier(), 
        ColumnOptions().setColor(BG_COLOR), 
        contains {
            row(
                Modifier()
                    .setHeight(96_px)
                    .setOuterPadding(8.f),
                panelRow,
                contains {}
            ),
            
            row(
                Modifier(),
                RowOptions(),
                contains {
                    // "Filebrowser"
                    column(
                        Modifier()
                            .setOuterPadding(8.f)
                            .setWidth(320_px),
                        ColumnOptions()
                            .setColor(CONT_COLOR)
                            .setRounding(ROUNDING)
                            .setScrollable(true)
                            .setScrollSpeed(40.f),
                        contains {
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions().setColor({55,58,74}).setRounding(4.f), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("  include/").setCharSize(16).setColor({180,190,220}).setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("    UILO.hpp").setCharSize(16).setColor({140,148,180}).setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("    UILO.cpp").setCharSize(16).setColor({140,148,180}).setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("    Page.hpp").setCharSize(16).setColor({140,148,180}).setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("    Page.cpp").setCharSize(16).setColor({140,148,180}).setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions().setColor({55,58,74}).setRounding(4.f), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("    elements/").setCharSize(16).setColor({180,190,220}).setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("      Element.hpp").setCharSize(16).setColor({140,148,180}).setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("      Element.cpp").setCharSize(16).setColor({140,148,180}).setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("      Elements.hpp").setCharSize(16).setColor({140,148,180}).setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("      Factory.hpp").setCharSize(16).setColor({140,148,180}).setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("      Modifier.hpp").setCharSize(16).setColor({140,148,180}).setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("      Modifier.cpp").setCharSize(16).setColor({140,148,180}).setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions().setColor({55,58,74}).setRounding(4.f), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("      containers/").setCharSize(16).setColor({180,190,220}).setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("        Container.hpp").setCharSize(16).setColor({140,148,180}).setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("        Container.cpp").setCharSize(16).setColor({140,148,180}).setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("        Column.hpp").setCharSize(16).setColor({140,148,180}).setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("        Column.cpp").setCharSize(16).setColor({140,148,180}).setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("        Row.hpp").setCharSize(16).setColor({140,148,180}).setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("        Row.cpp").setCharSize(16).setColor({140,148,180}).setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions().setColor({55,58,74}).setRounding(4.f), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("      decoration/").setCharSize(16).setColor({180,190,220}).setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("        Image.hpp").setCharSize(16).setColor({140,148,180}).setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("        Image.cpp").setCharSize(16).setColor({140,148,180}).setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("        Spacer.hpp").setCharSize(16).setColor({140,148,180}).setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("        Spacer.cpp").setCharSize(16).setColor({140,148,180}).setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("        Text.hpp").setCharSize(16).setColor({140,148,180}).setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("        Text.cpp").setCharSize(16).setColor({140,148,180}).setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions().setColor({55,58,74}).setRounding(4.f), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("      interactible/").setCharSize(16).setColor({180,190,220}).setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("        Button.hpp").setCharSize(16).setColor({140,148,180}).setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("        Button.cpp").setCharSize(16).setColor({140,148,180}).setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("        Dropdown.hpp").setCharSize(16).setColor({140,148,180}).setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("        Dropdown.cpp").setCharSize(16).setColor({140,148,180}).setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("        Knob.hpp").setCharSize(16).setColor({140,148,180}).setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("        Knob.cpp").setCharSize(16).setColor({140,148,180}).setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("        Slider.hpp").setCharSize(16).setColor({140,148,180}).setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("        Slider.cpp").setCharSize(16).setColor({140,148,180}).setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("        TextBox.hpp").setCharSize(16).setColor({140,148,180}).setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("        TextBox.cpp").setCharSize(16).setColor({140,148,180}).setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions().setColor({55,58,74}).setRounding(4.f), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("      utils/").setCharSize(16).setColor({180,190,220}).setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("        Alignment.hpp").setCharSize(16).setColor({140,148,180}).setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("        Dimension.hpp").setCharSize(16).setColor({140,148,180}).setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("        RenderUtils.hpp").setCharSize(16).setColor({140,148,180}).setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("        Timer.hpp").setCharSize(16).setColor({140,148,180}).setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("        Utils.hpp").setCharSize(16).setColor({140,148,180}).setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions().setColor({55,58,74}).setRounding(4.f), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("  assets/").setCharSize(16).setColor({180,190,220}).setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("    EmbeddedAssets.hpp").setCharSize(16).setColor({140,148,180}).setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("    EmbeddedFont.hpp").setCharSize(16).setColor({140,148,180}).setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("    EmbeddedIcons.hpp").setCharSize(16).setColor({140,148,180}).setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("  Makefile").setCharSize(16).setColor({140,148,180}).setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("  build.sh").setCharSize(16).setColor({140,148,180}).setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("  README.md").setCharSize(16).setColor({140,148,180}).setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("  TODO.txt").setCharSize(16).setColor({140,148,180}).setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("  License.txt").setCharSize(16).setColor({140,148,180}).setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("  UILO_NEW.md").setCharSize(16).setColor({140,148,180}).setTextAlignY(Align::CenterY)) }),
                        }, "1"
                    ),

                    // resizer(...)
                    resizer(
                        Modifier()
                            .setWidth(48_px), 
                        ResizerOptions()
                            .setDirection(ResizerDir::Left)
                            .setResizeWidthMin(10_pct)
                            .setResizeWidthMax(50_pct)
                    ),

                    column(
                        Modifier()
                            .setOuterPadding(8.f),
                        panelCol,
                        contains{
                            button(
                                Modifier()
                                    .setAlign(Align::CenterX | Align::CenterY)
                                    .setWidth(192_px)
                                    .setHeight(64_px)
                                    .setOnLeftClick([&](){ std::cout << "Test button clicked!!!" << std::endl; }),
                                ButtonOptions()
                                    .setColor({151, 120, 206})
                                    .setRounding(ROUNDING)
                                    .setLabel(
                                        text(
                                            Modifier()
                                                .setAlign(Align::CenterX | Align::CenterY),
                                            TextOptions()
                                                .setFont("assets/fonts/Montserrat.ttf")
                                                .setContent("TEST")
                                                .setColor(Color::White)
                                                .setTextAlignX(Align::CenterX)
                                                .setTextAlignY(Align::CenterY)
                                        )
                                    ),
                                "test_button"
                            ),
                            spacer(Modifier().setHeight(16_px).setAlign(Align::CenterY)),
                            dropdown(
                                Modifier()
                                    .setAlign(Align::CenterX | Align::CenterY)
                                    .setWidth(256_px)
                                    .setHeight(32_px),
                                DropdownOptions()
                                    .setFont("assets/fonts/Montserrat.ttf")
                                    .setPopupRounding(ROUNDING)
                                    .setHeaderRounding(ROUNDING)
                                    .setPlaceholder("Choose...")
                                    .setSpacer(4.f)
                                    .setHeaderTextAlignment(Align::CenterX, Align::CenterY)
                                    .setPopupTextAlignment(Align::CenterX, Align::CenterY)
                                    .setMaxItems(6)
                                    .setDividerColor({60, 60, 60})
                                    .setDividerThickness(1.f)
                                    .setOnItemChanged([&](const std::string& s){ std::cout << "Dropdown changed to: " << s << std::endl; }),
                                { "Column", "Row", "Button", "Dropdown", "Knob", "Slider", "1000", "2000", "3000", "4000", "5000", "6000"}
                            ),
                            spacer(Modifier().setHeight(16_px).setAlign(Align::CenterY)),
                            textbox(
                                Modifier()
                                    .setAlign(Align::CenterX | Align::CenterY)
                                    .setWidth(512_px)
                                    .setHeight(48_px),
                                TextboxOptions()
                                    .setCharSize(32)
                                    .setFont("assets/fonts/Montserrat.ttf")
                                    .setRounding(ROUNDING)
                                    .setPlaceholder("Type Something...")
                                    .setBackgroundColor({100, 100, 100})
                                    .setMultiline(true)                                    
                                    .setPaddingLeft(16.f)
                                    .setPaddingRight(16.f)
                                    .setOutlineColor({151, 120, 206})
                                    .setOutlineThickness(2.f)
                                    .setMaxResizeLines(6)
                            )
                        }, "2"
                    )
                }
            ),

            resizer(
                Modifier()
                    .setHeight(48_px), 
                ResizerOptions()
                    .setDirection(ResizerDir::Bottom)
                    .setResizeHeightMin(10_pct)
                    .setResizeHeightMax(50_pct)
            ),

            row(
                Modifier()
                    .setHeight(256_px)
                    .setOuterPadding(8.f),
                panelRow,
                contains {
                    slider(
                        Modifier()
                            .setHeight(32_px)
                            .setAlign(Align::CenterX | Align::CenterY), 
                        SliderOptions()
                            .setThumbShape(ThumbShape::Rect)
                            .setThumbSize(16, 48)
                            .setOnValueChanged([&](float v){std::cout << "Value to " << v << std::endl; })
                            .setFillColor({151, 120, 206})
                            .setThumbRounding(ROUNDING)
                            .setStep(0.01f),
                        ""
                    )
                }
            ),
        }, "root"
    );
}