#include "../include/UILO.hpp"
#include "../include/renderer/Renderer.hpp"
#include <SDL3/SDL.h>
#include <bgfx/bgfx.h>
#include <iostream>
#include <cstdio>

using namespace uilo;

float ROUNDING = 8.f;

Container* buildRootContainer(UILO& ui);

static Palette makeDarkPalette() {
    Palette p;
    p.set("app.bg",                 {33,  35,  47,  255});
    p.set("panel",                  {44,  47,  60,  255});
    p.set("panelAlt",               {55,  58,  74,  255});
    p.set("text",                   {180, 190, 220, 255});
    p.set("textDim",                {140, 148, 180, 255});
    p.set("outline",                {80,  84,  100, 255});
    p.set("divider",                {60,  60,  60,  255});
    p.set("accent",                 {151, 120, 206, 255});
    p.set("accentHover",            {171, 140, 226, 255});
    p.set("accent.green",           {120, 200, 170, 255});
    p.set("accent.red",             {220, 120, 120, 255});
    p.set("onAccent",               {255, 255, 255, 255});
    p.set("knob.body",              {44,  47,  60,  255});
    p.set("knob.track",             {30,  32,  42,  255});
    p.set("knob.indicator",         {255, 255, 255, 255});
    p.set("textbox.bg",             {100, 100, 100, 255});
    p.set("textbox.text",           {255, 255, 255, 255});
    p.set("textbox.placeholder",    {200, 200, 200, 255});
    p.set("textbox.cursor",         {255, 255, 255, 255});
    p.set("textbox.selection",      {151, 120, 206, 120});
    return p;
}

static Palette makeLightPalette() {
    Palette p;
    p.set("app.bg",                 {235, 238, 245, 255});
    p.set("panel",                  {250, 251, 254, 255});
    p.set("panelAlt",               {220, 225, 238, 255});
    p.set("text",                   {40,  46,  72,  255});
    p.set("textDim",                {95,  104, 132, 255});
    p.set("outline",                {180, 186, 205, 255});
    p.set("divider",                {200, 205, 220, 255});
    p.set("accent",                 {120, 90,  190, 255});
    p.set("accentHover",            {140, 110, 210, 255});
    p.set("accent.green",           {70,  160, 130, 255});
    p.set("accent.red",             {200, 80,  80,  255});
    p.set("onAccent",               {255, 255, 255, 255});
    p.set("knob.body",              {250, 251, 254, 255});
    p.set("knob.track",             {215, 220, 235, 255});
    p.set("knob.indicator",         {40,  46,  72,  255});
    p.set("textbox.bg",             {220, 224, 236, 255});
    p.set("textbox.text",           {40,  46,  72,  255});
    p.set("textbox.placeholder",    {130, 138, 165, 255});
    p.set("textbox.cursor",         {40,  46,  72,  255});
    p.set("textbox.selection",      {120, 90,  190, 110});
    return p;
}

static void applyTheme(UILO& ui, bool dark) {
    ui.setPalette(dark ? makeDarkPalette() : makeLightPalette());
}

// ---------------------------------------------------------------------------
// Floating FPS HUD
// ---------------------------------------------------------------------------
struct FpsHud {
    Text*   fps   = nullptr;
    Text*   draws = nullptr;
    Text*   cpu   = nullptr;
    Text*   gpu   = nullptr;
    Column* root  = nullptr;
};

static FpsHud installFpsHud(UILO& ui) {
    ui.addFloating(freeColumn(
        Modifier()
            .setWidth(180_px)
            .setMaterial(
                Material::Blur()
                    .setRadius(2.f)
            )
            .setHeight(96_px),
        ColumnOptions()
            .setColorRole("panelAlt")
            .setRounding(ROUNDING),
        contains{
            text(
                Modifier().setAlign(Align::Left | Align::CenterY),
                TextOptions()
                    .setFont("assets/fonts/Montserrat.ttf")
                    .setContent(" FPS:")
                    .setColorRole("text")
                    .setCharSize(18)
                    .setTextAlignY(Align::CenterY),
                "fps_text"
            ),
            text(
                Modifier().setAlign(Align::Left | Align::CenterY),
                TextOptions()
                    .setFont("assets/fonts/Montserrat.ttf")
                    .setContent(" draws:")
                    .setColorRole("textDim")
                    .setCharSize(14)
                    .setTextAlignY(Align::CenterY),
                "fps_draws"
            ),
            text(
                Modifier().setAlign(Align::Left | Align::CenterY),
                TextOptions()
                    .setFont("assets/fonts/Montserrat.ttf")
                    .setContent(" cpu:")
                    .setColorRole("textDim")
                    .setCharSize(14)
                    .setTextAlignY(Align::CenterY),
                "fps_cpu"
            ),
            text(
                Modifier().setAlign(Align::Left | Align::CenterY),
                TextOptions()
                    .setFont("assets/fonts/Montserrat.ttf")
                    .setContent(" gpu:")
                    .setColorRole("textDim")
                    .setCharSize(14)
                    .setTextAlignY(Align::CenterY),
                "fps_gpu"
            )
        },
        "fps_hud"
    ).setPosition(12_px, 12_px).setDraggable(true));
    return FpsHud{
        ui.getElement<Text>  ("fps_text"),
        ui.getElement<Text>  ("fps_draws"),
        ui.getElement<Text>  ("fps_cpu"),
        ui.getElement<Text>  ("fps_gpu"),
        ui.getElement<Column>("fps_hud"),
    };
}

static void updateFpsHud(const FpsHud& hud, const Renderer& renderer, float fpsValue) {
    const RendererStats st = renderer.getStats();
    char buf[64];
    if (hud.fps)   { std::snprintf(buf, sizeof(buf), " FPS: %.0f",    fpsValue);     hud.fps  ->setString(buf); }
    if (hud.draws) { std::snprintf(buf, sizeof(buf), " draws: %u",    st.numDraw);   hud.draws->setString(buf); }
    if (hud.cpu)   { std::snprintf(buf, sizeof(buf), " cpu: %.2f ms", st.cpuTimeMs); hud.cpu  ->setString(buf); }
    if (hud.gpu)   { std::snprintf(buf, sizeof(buf), " gpu: %.2f ms", st.gpuTimeMs); hud.gpu  ->setString(buf); }
}

// ---------------------------------------------------------------------------
// Keybinds
// ---------------------------------------------------------------------------
// Bound by semantic name and polled by UILO::update(). `true` at the end means
// press-edge only, which is what the hand-rolled key latches used to do.
// UILO suppresses these while a Textbox has focus, so typing "v" is just a "v".
static void installKeybinds(UILO& ui, Renderer& renderer, Column* fpsHud) {
    Keybinds& keys = ui.getKeybinds();

    keys.bindAction("ui.scaleUp", { SDL_SCANCODE_EQUALS, SDL_SCANCODE_KP_PLUS },
        [&ui] { ui.setScale(ui.getScale() + 0.1f); }, true);

    keys.bindAction("ui.scaleDown", { SDL_SCANCODE_MINUS, SDL_SCANCODE_KP_MINUS },
        [&ui] { ui.setScale(ui.getScale() - 0.1f); }, true);

    keys.bindAction("ui.toggleFpsHud", SDL_SCANCODE_F10,
        [fpsHud] {
            if (!fpsHud) return;
            Modifier& m = fpsHud->getModifier();
            m.setVisible(!m.getVisible());
        }, true);

    keys.bindAction("renderer.toggleVsync", SDL_SCANCODE_V,
        [&renderer] { renderer.setVsync(!renderer.getVsync()); }, true);

    keys.bindAction("app.quit", SDL_SCANCODE_ESCAPE, [&ui] { ui.quit(); }, true);
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------
int main() {
    Renderer renderer;
    if (!renderer.init(1280, 720, "Containers", 16)) {
        std::fprintf(stderr, "Failed to initialize renderer\n");
        return 1;
    }

    UILO ui;
    ui.setRenderer(renderer);
    ui.addPage(page(buildRootContainer(ui), "main_page"));
    ui.setPage("main_page");
    ui.setScale(OS::scale());
    applyTheme(ui, true);

    FpsHud hud = installFpsHud(ui);
    if (hud.root) hud.root->getModifier().setVisible(false);
    installKeybinds(ui, renderer, hud.root);

    std::fprintf(
        stderr, "[UILO] bgfx renderer: %s\n",
        bgfx::getRendererName(bgfx::getCaps()->rendererType)
    );

    while (ui.isRunning()) {
        ui.pollEvents();
        ui.update();

        if (hud.root && hud.root->getModifier().getVisible())
            updateFpsHud(hud, renderer, ui.getAvgFrameRate());

        renderer.beginFrame();
        renderer.clear(ui.getPalette().get("app.bg"));
        ui.render();
        renderer.endFrame();
    }
    return 0;
}

Container* buildRootContainer(UILO& ui) {
    auto panelCol = ColumnOptions().setColorRole("panel").setRounding(ROUNDING);
    auto panelRow = RowOptions().setColorRole("panel").setRounding(ROUNDING);

    return column(
        Modifier(),
        ColumnOptions().setColorRole("app.bg"),
        contains {
            row(
                Modifier()
                    .setHeight(96_px)
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
                            .setFillColorRole("accent")
                            .setThumbColorRole("text")
                            .setTrackColorRole("panelAlt")
                            .setThumbRounding(ROUNDING)
                            .setDefaultValue(0.5f),
                        "main_slider"
                    )
                }
            ),
            
            row(
                Modifier(),
                RowOptions(),
                contains {
                    filebrowser(
                        Modifier()
                            .setOuterPadding(8.f)
                            .setWidth(320_px),
                        FileBrowserOptions()
                            .setRootPath("../")
                            .setFont("assets/fonts/Montserrat.ttf")
                            .setOnFileClicked([](const std::filesystem::path& p) {
                                std::cout << "File: " << p.string() << std::endl;
                            })
                            .setOnDirectoryExpanded([](const std::filesystem::path& p) {
                                std::cout << "Expanded: " << p.string() << std::endl;
                            }),
                        "file_browser"
                    ),

                    resizer(
                        Modifier()
                            .setWidth(48_px)
                            .setOnUpdateEnd([alphaF = 0.f](Resizer* r) mutable {
                                constexpr Color target = {255, 255, 255, 100};
                                constexpr float fadeSec = 0.18f;
                                const bool active = r->isHovered() || r->isDragging();
                                const float step = (r->getDeltaTime() / fadeSec) * (float)target.a;
                                alphaF += active ? +step : -step;
                                if (alphaF < 0.f) alphaF = 0.f;
                                if (alphaF > (float)target.a) alphaF = (float)target.a;
                                Color c = r->getOptions().getColor();
                                c.r = target.r; c.g = target.g; c.b = target.b;
                                c.a = (uint8_t)(alphaF + 0.5f);
                                r->getOptions().setColor(c);
                            }),
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
                                    .setOnLeftClick([&](Button* b){ std::cout << "Test button clicked!!!" << std::endl; })
                                    .setOnHoverEnter([](Button* b){ b->getOptions().setColorRole("accentHover"); })
                                    .setOnHoverExit([](Button* b){ b->getOptions().setColorRole("accent"); }),
                                ButtonOptions()
                                    .setColorRole("accent")
                                    .setRounding(ROUNDING)
                                    .setLabel(
                                        text(
                                            Modifier()
                                                .setAlign(Align::CenterX | Align::CenterY),
                                            TextOptions()
                                                .setFont("assets/fonts/Montserrat.ttf")
                                                .setContent("TEST")
                                                .setColorRole("onAccent")
                                                .setTextAlignX(Align::CenterX)
                                                .setTextAlignY(Align::CenterY)
                                        )
                                    ),
                                "test_button"
                            ),
                            spacer(Modifier().setHeight(16_px).setAlign(Align::CenterY)),
                            row(
                                Modifier()
                                    .setHeight(96_px)
                                    .setAlign(Align::CenterX | Align::CenterY),
                                RowOptions(),
                                contains {
                                    knob(
                                        Modifier()
                                            .setWidth(96_px)
                                            .setHeight(96_px)
                                            .setAlign(Align::CenterX | Align::CenterY),
                                        KnobOptions()
                                            .setBodyColorRole("knob.body")
                                            .setOutlineColorRole("outline")
                                            .setOutlineThickness(1.f)
                                            .setTrackColorRole("knob.track")
                                            .setArcColorRole("accent")
                                            .setIndicatorColorRole("knob.indicator")
                                            .setArcThickness(8.f)
                                            .setArcGap(4.f)
                                            .setIndicatorThickness(4.f)
                                            .setIndicatorInset(0.35f)
                                            .setIndicatorLength(0.85f)
                                            .setDefaultValue(0.5f)
                                            .setOnValueChanged([](float v){
                                                std::cout << "Knob: " << v << std::endl;
                                            }),
                                        "knob_a"
                                    ),
                                    spacer(Modifier().setWidth(16_px).setAlign(Align::CenterX)),
                                    knob(
                                        Modifier()
                                            .setWidth(96_px)
                                            .setHeight(96_px)
                                            .setAlign(Align::CenterX | Align::CenterY),
                                        KnobOptions()
                                            .setBodyColorRole("knob.body")
                                            .setOutlineColorRole("outline")
                                            .setOutlineThickness(1.f)
                                            .setTrackColorRole("knob.track")
                                            .setArcColorRole("accent.green")
                                            .setIndicatorColorRole("knob.indicator")
                                            .setStartAngle(180.f)
                                            .setEndAngle(0.f)
                                            .setRange(-1.f, 1.f)
                                            .setDefaultValue(0.f),
                                        "knob_pan"
                                    )
                                }
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
                                    .setPlaceholder("Theme: Dark")
                                    .setSpacer(4.f)
                                    .setPopupTextAlignment(Align::CenterX, Align::CenterY)
                                    .setHeaderColorRole("panelAlt")
                                    .setHeaderTextColorRole("text")
                                    .setPopupColorRole("panel")
                                    .setItemColorRole("panel")
                                    .setItemHoverColorRole("panelAlt")
                                    .setTextColorRole("text")
                                    .setDividerColorRole("divider")
                                    .setDividerThickness(1.f)
                                    .setOnItemChanged([&ui](const std::string& s){
                                        if      (s == "Dark")  applyTheme(ui, true);
                                        else if (s == "Light") applyTheme(ui, false);
                                    }),
                                { "Dark", "Light" },
                                "theme_dropdown"
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
                                    .setBackgroundColorRole("textbox.bg")
                                    .setTextColorRole("textbox.text")
                                    .setPlaceholderColorRole("textbox.placeholder")
                                    .setCursorColorRole("textbox.cursor")
                                    .setSelectionColorRole("textbox.selection")
                                    .setMultiline(true)
                                    .setPaddingLeft(16.f)
                                    .setPaddingRight(16.f)
                                    .setOutlineColorRole("accent")
                                    .setOutlineThickness(2.f)
                                    .setMaxResizeLines(6)
                                    .setOnEnterPressed([&](const std::string& s){ std::cout << "TextBox: " << s << std::endl;}),
                                "main_textbox"
                            ),
                            spacer(Modifier().setHeight(16_px).setAlign(Align::CenterY)),
                            image(
                                Modifier()
                                    .setWidth(256_px)
                                    .setAlign(Align::CenterX | Align::CenterY),
                                ImageOptions()
                                    .setClipEllipse(true)
                                    .setPath("assets/images/stones.jpg")
                                    .setLockAspectWidth(true)
                            )
                        }, "2"
                    )
                }
            ),

            resizer(
                Modifier()
                    .setHeight(48_px)
                    .setOnUpdateEnd([alphaF = 0.f](Resizer* r) mutable {
                        constexpr Color target = {255, 255, 255, 100};
                        constexpr float fadeSec = 0.18f;
                        const bool active = r->isHovered() || r->isDragging();
                        const float step = (r->getDeltaTime() / fadeSec) * (float)target.a;
                        alphaF += active ? +step : -step;
                        if (alphaF < 0.f) alphaF = 0.f;
                        if (alphaF > (float)target.a) alphaF = (float)target.a;
                        Color c = r->getOptions().getColor();
                        c.r = target.r; c.g = target.g; c.b = target.b;
                        c.a = (uint8_t)(alphaF + 0.5f);
                        r->getOptions().setColor(c);
                    }),
                ResizerOptions()
                    .setDirection(ResizerDir::Bottom)
                    .setResizeHeightMin(10_pct)
                    .setResizeHeightMax(50_pct)
            ),

            // Bottom bar: an icon strip. Names come from the generated
            // constants, so a typo is a compile error rather than a blank slot.
            row(
                Modifier()
                    .setHeight(256_px)
                    .setOuterPadding(8.f),
                panelRow,
                contains {
                    row(
                        Modifier().setHeight(48_px).setAlign(Align::CenterX | Align::CenterY),
                        RowOptions(),
                        contains {
                            // Palette-driven tint: one setter, no shader work.
                            icon(Modifier().setWidth(48_px).setAlign(Align::CenterY),
                                 IconOptions().setIcon(Resources::icons::folder_plus)
                                              .setColorRole("text")),
                            icon(Modifier().setWidth(48_px).setAlign(Align::CenterY),
                                 IconOptions().setIcon(Resources::icons::settings)
                                              .setColorRole("textDim")),
                            icon(Modifier().setWidth(48_px).setAlign(Align::CenterY),
                                 IconOptions().setIcon(Resources::icons::search)
                                              .setColorRole("accent")),
                            icon(Modifier().setWidth(48_px).setAlign(Align::CenterY),
                                 IconOptions().setIcon(Resources::icons::heart)
                                              .setColorRole("accent.red")),
                            icon(Modifier().setWidth(48_px).setAlign(Align::CenterY),
                                 IconOptions().setIcon(Resources::icons::check_circle)
                                              .setColorRole("accent.green")),
                            spacer(Modifier().setWidth(32_px)),
                            // Same icon, three stroke weights. Stroke width is in
                            // the icon's own 24-unit authoring space (stock = 1.5),
                            // so it stays proportional at any size.
                            icon(Modifier().setWidth(48_px).setAlign(Align::CenterY),
                                 IconOptions().setIcon(Resources::icons::zap)
                                              .setColorRole("text")
                                              .setStrokeWidth(0.75f)),
                            icon(Modifier().setWidth(48_px).setAlign(Align::CenterY),
                                 IconOptions().setIcon(Resources::icons::zap)
                                              .setColorRole("text")
                                              .setStrokeWidth(1.5f)),
                            icon(Modifier().setWidth(48_px).setAlign(Align::CenterY),
                                 IconOptions().setIcon(Resources::icons::zap)
                                              .setColorRole("text")
                                              .setStrokeWidth(3.f)),
                            spacer(Modifier().setWidth(32_px)),
                            // Clickable: the icon is decoration, so the row's
                            // handler is what fires (pointer-transparent child).
                            row(Modifier()
                                    .setWidth(64_px)
                                    .setOnLeftClick([](Element*) {
                                        std::cout << "Icon button clicked" << std::endl;
                                    })
                                    .setOnHoverEnter([](Row* r) { r->getOptions().setColorRole("panelAlt"); })
                                    .setOnHoverExit([](Row* r) { r->getOptions().setColorRole("none"); }),
                                RowOptions().setRounding(ROUNDING),
                                contains {
                                    icon(Modifier().setWidth(32_px)
                                             .setAlign(Align::CenterX | Align::CenterY),
                                         IconOptions().setIcon(Resources::icons::download)
                                                      .setColorRole("text"))
                                }
                            )
                        }
                    )
                }
            ),
        }, "root"
    );
}