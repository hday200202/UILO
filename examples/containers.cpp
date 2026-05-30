#include "../include/UILO.hpp"
#include "../include/renderer/Renderer.hpp"
#include <SDL3/SDL.h>
#include <bgfx/bgfx.h>
#include <iostream>
#include <cstdio>
#include <cmath>
#include <vector>

using namespace uilo;

float ROUNDING = 8.f;

Container* buildRootContainer(UILO& ui);

static Palette makeDarkPalette() {
    Palette p;
    p.set("app.bg",          {33,  35,  47,  255});
    p.set("panel",           {44,  47,  60,  255});
    p.set("panelAlt",        {55,  58,  74,  255});
    p.set("text",            {180, 190, 220, 255});
    p.set("textDim",         {140, 148, 180, 255});
    p.set("outline",         {80,  84,  100, 255});
    p.set("divider",         {60,  60,  60,  255});
    p.set("accent",          {151, 120, 206, 255});
    p.set("accentHover",     {171, 140, 226, 255});
    p.set("accent.green",    {120, 200, 170, 255});
    p.set("accent.red",      {220, 120, 120, 255});
    p.set("onAccent",        {255, 255, 255, 255});
    p.set("knob.body",       {44,  47,  60,  255});
    p.set("knob.track",      {30,  32,  42,  255});
    p.set("knob.indicator",  {255, 255, 255, 255});
    p.set("textbox.bg",      {100, 100, 100, 255});
    p.set("textbox.text",    {255, 255, 255, 255});
    p.set("textbox.placeholder", {200, 200, 200, 255});
    p.set("textbox.cursor",  {255, 255, 255, 255});
    p.set("textbox.selection", {151, 120, 206, 120});
    p.set("waveform.bg",     {0,   0,   0,   0});
    return p;
}

static Palette makeLightPalette() {
    Palette p;
    p.set("app.bg",          {235, 238, 245, 255});
    p.set("panel",           {250, 251, 254, 255});
    p.set("panelAlt",        {220, 225, 238, 255});
    p.set("text",            {40,  46,  72,  255});
    p.set("textDim",         {95,  104, 132, 255});
    p.set("outline",         {180, 186, 205, 255});
    p.set("divider",         {200, 205, 220, 255});
    p.set("accent",          {120, 90,  190, 255});
    p.set("accentHover",     {140, 110, 210, 255});
    p.set("accent.green",    {70,  160, 130, 255});
    p.set("accent.red",      {200, 80,  80,  255});
    p.set("onAccent",        {255, 255, 255, 255});
    p.set("knob.body",       {250, 251, 254, 255});
    p.set("knob.track",      {215, 220, 235, 255});
    p.set("knob.indicator",  {40,  46,  72,  255});
    p.set("textbox.bg",      {220, 224, 236, 255});
    p.set("textbox.text",    {40,  46,  72,  255});
    p.set("textbox.placeholder", {130, 138, 165, 255});
    p.set("textbox.cursor",  {40,  46,  72,  255});
    p.set("textbox.selection", {120, 90,  190, 110});
    p.set("waveform.bg",     {0,   0,   0,   0});
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
                    .setTextAlignX(Align::Left)
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
                    .setTextAlignX(Align::Left)
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
                    .setTextAlignX(Align::Left)
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
                    .setTextAlignX(Align::Left)
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
// Waveform demo data + zoom binding
// ---------------------------------------------------------------------------
static void installWaveformDemo(UILO& ui) {
    auto* wf = ui.getElement<Waveform>("main_waveform");
    if (!wf) return;

    constexpr float    kSampleRate = 48000.f;
    constexpr float    kDuration   = 4.0f; // seconds
    const std::size_t  kFrames     = (std::size_t)(kSampleRate * kDuration);
    std::vector<float> bufL(kFrames), bufR(kFrames);
    for (std::size_t i = 0; i < kFrames; ++i) {
        const float t  = (float)i / kSampleRate;
        // Two sine partials + a slow tremolo envelope + a little drift
        // between L/R so the channels look different in Stacked layout.
        const float env  = 0.5f * (1.f + std::sin(2.f * 3.14159265f * 0.8f * t));
        const float fade = std::min(1.f, t / 0.05f) *
                           std::min(1.f, (kDuration - t) / 0.4f);
        const float wL = std::sin(2.f * 3.14159265f * 220.f * t)
                       + 0.5f * std::sin(2.f * 3.14159265f * 440.f * t);
        const float wR = std::sin(2.f * 3.14159265f * 246.94f * t)
                       + 0.4f * std::sin(2.f * 3.14159265f * 523.25f * t);
        bufL[i] = 0.55f * fade * env * wL;
        bufR[i] = 0.55f * fade * env * wR;
    }
    const float* channels[2] = { bufL.data(), bufR.data() };
    wf->setSamples(channels, 2, kFrames);

    // Ctrl + scroll wheel zooms the waveform around the mouse x.
    wf->getModifier().setOnScroll([&ui, wf, anchor = 0.f](Element* self, float delta) mutable {
        const SDL_Keymod mods = SDL_GetModState();
        if (!(mods & SDL_KMOD_CTRL)) return;
        const Rectf b = self->getBounds();
        if (b.size.x <= 0.f) return;
        // Lock the zoom pivot to where the cursor was when the gesture
        // started; reusing the live cursor during momentum coast would
        // jitter the pivot by sub-pixel noise each tick.
        if (!ui.isMomentumScrolling()) {
            const float mx = ui.getMousePosition().x;
            anchor = std::max(0.f, std::min(1.f, (mx - b.position.x) / b.size.x));
        }
        const float factor = std::pow(1.2f, delta);
        wf->zoomAt(anchor, factor);
    });
}

// ---------------------------------------------------------------------------
// Input + main loop
// ---------------------------------------------------------------------------
struct InputLatch {
    bool plus  = false;
    bool minus = false;
    bool f10   = false;
    bool v     = false;
};

static void handleEvent(
    SDL_Event&      event,
    InputLatch&     latch,
    bool&           showFps,
    bool&           running,
    UILO&           ui,
    Renderer&       renderer
) {
    if (event.type == SDL_EVENT_QUIT) running = false;
    if (event.type == SDL_EVENT_KEY_DOWN) {
        SDL_Keycode k = event.key.key;
        if (k == SDLK_EQUALS || k == SDLK_KP_PLUS) {
            if (!latch.plus) ui.setScale(ui.getScale() + 0.1f);
            latch.plus = true;
        } else if (k == SDLK_MINUS || k == SDLK_KP_MINUS) {
            if (!latch.minus) ui.setScale(ui.getScale() - 0.1f);
            latch.minus = true;
        } else if (k == SDLK_F10) {
            if (!latch.f10) showFps = !showFps;
            latch.f10 = true;
        } else if (k == SDLK_V) {
            if (!latch.v) renderer.setVsync(!renderer.getVsync());
            latch.v = true;
        }
    } else if (event.type == SDL_EVENT_KEY_UP) {
        SDL_Keycode k = event.key.key;
        if (k == SDLK_EQUALS || k == SDLK_KP_PLUS)  latch.plus  = false;
        if (k == SDLK_MINUS  || k == SDLK_KP_MINUS) latch.minus = false;
        if (k == SDLK_F10)                          latch.f10   = false;
        if (k == SDLK_V)                            latch.v     = false;
    }
    ui.handleEvent(event);
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
    ui.setScale(1.5f);
    applyTheme(ui, true);

    FpsHud hud = installFpsHud(ui);
    installWaveformDemo(ui);

    std::fprintf(
        stderr, "[UILO] bgfx renderer: %s\n",
        bgfx::getRendererName(bgfx::getCaps()->rendererType)
    );

    InputLatch latch;
    bool  showFps   = false;
    bool  running   = true;
    float fpsTimer  = 0.f;
    int   fpsLoops  = 0;
    float fpsValue  = 0.f;

    auto poll = [&]() {
        SDL_Event event;
        while (SDL_PollEvent(&event))
            handleEvent(event, latch, showFps, running, ui, renderer);
    };

    auto renderOnce = [&]() {
        renderer.beginFrame();
        renderer.clear(ui.getPalette().get("app.bg"));
        ui.render();
        renderer.endFrame();
    };

    while (running) {
        const float dt = ui.getDeltaTime();
        fpsTimer += dt;
        fpsLoops++;
        if (fpsTimer >= 0.25f) {
            fpsValue = (float)fpsLoops / fpsTimer;
            fpsLoops = 0;
            fpsTimer = 0.f;
            updateFpsHud(hud, renderer, fpsValue);
        }
        if (hud.root) hud.root->getModifier().setVisible(showFps);

        poll();
        ui.update();

        renderOnce();
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
                    // "Filebrowser"
                    column(
                        Modifier()
                            .setOuterPadding(8.f)
                            .setWidth(320_px),
                        ColumnOptions()
                            .setColorRole("panel")
                            .setRounding(ROUNDING)
                            .setScrollable(true)
                            .setScrollSpeed(60.f),
                        contains {
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions().setColorRole("panelAlt").setRounding(4.f), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("  include/").setCharSize(16).setColorRole("text").setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("    UILO.hpp").setCharSize(16).setColorRole("textDim").setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("    UILO.cpp").setCharSize(16).setColorRole("textDim").setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("    Page.hpp").setCharSize(16).setColorRole("textDim").setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("    Page.cpp").setCharSize(16).setColorRole("textDim").setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions().setColorRole("panelAlt").setRounding(4.f), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("    elements/").setCharSize(16).setColorRole("text").setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("      Element.hpp").setCharSize(16).setColorRole("textDim").setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("      Element.cpp").setCharSize(16).setColorRole("textDim").setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("      Elements.hpp").setCharSize(16).setColorRole("textDim").setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("      Factory.hpp").setCharSize(16).setColorRole("textDim").setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("      Modifier.hpp").setCharSize(16).setColorRole("textDim").setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("      Modifier.cpp").setCharSize(16).setColorRole("textDim").setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions().setColorRole("panelAlt").setRounding(4.f), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("      containers/").setCharSize(16).setColorRole("text").setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("        Container.hpp").setCharSize(16).setColorRole("textDim").setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("        Container.cpp").setCharSize(16).setColorRole("textDim").setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("        Column.hpp").setCharSize(16).setColorRole("textDim").setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("        Column.cpp").setCharSize(16).setColorRole("textDim").setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("        Row.hpp").setCharSize(16).setColorRole("textDim").setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("        Row.cpp").setCharSize(16).setColorRole("textDim").setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions().setColorRole("panelAlt").setRounding(4.f), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("      decoration/").setCharSize(16).setColorRole("text").setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("        Image.hpp").setCharSize(16).setColorRole("textDim").setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("        Image.cpp").setCharSize(16).setColorRole("textDim").setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("        Spacer.hpp").setCharSize(16).setColorRole("textDim").setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("        Spacer.cpp").setCharSize(16).setColorRole("textDim").setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("        Text.hpp").setCharSize(16).setColorRole("textDim").setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("        Text.cpp").setCharSize(16).setColorRole("textDim").setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions().setColorRole("panelAlt").setRounding(4.f), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("      interactible/").setCharSize(16).setColorRole("text").setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("        Button.hpp").setCharSize(16).setColorRole("textDim").setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("        Button.cpp").setCharSize(16).setColorRole("textDim").setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("        Dropdown.hpp").setCharSize(16).setColorRole("textDim").setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("        Dropdown.cpp").setCharSize(16).setColorRole("textDim").setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("        Knob.hpp").setCharSize(16).setColorRole("textDim").setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("        Knob.cpp").setCharSize(16).setColorRole("textDim").setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("        Slider.hpp").setCharSize(16).setColorRole("textDim").setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("        Slider.cpp").setCharSize(16).setColorRole("textDim").setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("        TextBox.hpp").setCharSize(16).setColorRole("textDim").setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("        TextBox.cpp").setCharSize(16).setColorRole("textDim").setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions().setColorRole("panelAlt").setRounding(4.f), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("      utils/").setCharSize(16).setColorRole("text").setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("        Alignment.hpp").setCharSize(16).setColorRole("textDim").setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("        Dimension.hpp").setCharSize(16).setColorRole("textDim").setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("        RenderUtils.hpp").setCharSize(16).setColorRole("textDim").setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("        Timer.hpp").setCharSize(16).setColorRole("textDim").setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("        Utils.hpp").setCharSize(16).setColorRole("textDim").setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions().setColorRole("panelAlt").setRounding(4.f), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("  assets/").setCharSize(16).setColorRole("text").setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("    EmbeddedAssets.hpp").setCharSize(16).setColorRole("textDim").setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("    EmbeddedFont.hpp").setCharSize(16).setColorRole("textDim").setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("    EmbeddedIcons.hpp").setCharSize(16).setColorRole("textDim").setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("  Makefile").setCharSize(16).setColorRole("textDim").setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("  build.sh").setCharSize(16).setColorRole("textDim").setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("  README.md").setCharSize(16).setColorRole("textDim").setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("  TODO.txt").setCharSize(16).setColorRole("textDim").setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("  License.txt").setCharSize(16).setColorRole("textDim").setTextAlignY(Align::CenterY)) }),
                            row(Modifier().setHeight(36_px).setOuterPadding(4.f), RowOptions(), contains { text(Modifier(), TextOptions().setFont("assets/fonts/Montserrat.ttf").setContent("  UILO_NEW.md").setCharSize(16).setColorRole("textDim").setTextAlignY(Align::CenterY)) }),
                        }, "1"
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

#if 1
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
                                            .setRange(0.f, 1.f)
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
                                            .setArcDirection(KnobArcDir::CounterClockwise)
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
                                    .setHeaderTextAlignment(Align::CenterX, Align::CenterY)
                                    .setPopupTextAlignment(Align::CenterX, Align::CenterY)
                                    .setMaxItems(6)
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
#else
                    [&]() -> Element* {
                        auto* c = canvas(
                            Modifier()
                                .setOuterPadding(8.f),
                            CanvasOptions()
                                .setColorRole("panel")
                                .setRounding(ROUNDING)
                                .setGridSize(32.f, 32.f)
                                .setGridLineStyle(GridLineStyle::Dots)
                                .setGridLineColorRole("textDim")
                                .setGridLineThickness(1.5f)
                                .setGridLineSpacing(1)
                                .setScrollSpeed(80.f)
                                .setMinX(0.f)
                                .setMinY(0.f)
                                .setZoomAxes(true, true),
                            contains{}, "canvas_panel"
                        );

                        c->addChild(
                            button(
                                Modifier()
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
                                            Modifier().setAlign(Align::CenterX | Align::CenterY),
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
                            32, 32
                        );

                        c->addChild(
                            knob(
                                Modifier().setWidth(96_px).setHeight(96_px),
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
                                    .setRange(0.f, 1.f)
                                    .setDefaultValue(0.5f)
                                    .setOnValueChanged([](float v){ std::cout << "Knob: " << v << std::endl; }),
                                "knob_a"
                            ),
                            32, 128
                        );

                        c->addChild(
                            knob(
                                Modifier().setWidth(96_px).setHeight(96_px),
                                KnobOptions()
                                    .setBodyColorRole("knob.body")
                                    .setOutlineColorRole("outline")
                                    .setOutlineThickness(1.f)
                                    .setTrackColorRole("knob.track")
                                    .setArcColorRole("accent.green")
                                    .setIndicatorColorRole("knob.indicator")
                                    .setStartAngle(180.f)
                                    .setEndAngle(0.f)
                                    .setArcDirection(KnobArcDir::CounterClockwise)
                                    .setRange(-1.f, 1.f)
                                    .setDefaultValue(0.f),
                                "knob_pan"
                            ),
                            160, 128
                        );

                        c->addChild(
                            textbox(
                                Modifier().setWidth(512_px).setHeight(48_px),
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
                            32, 256
                        );

                        c->addChild(
                            image(
                                Modifier().setWidth(256_px).setHeight(256_px),
                                ImageOptions()
                                    .setClipEllipse(true)
                                    .setPath("assets/images/stones.jpg")
                                    .setLockAspectWidth(true)
                            ),
                            32, 352
                        );

                        return c;
                    }()
#endif
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

            row(
                Modifier()
                    .setHeight(256_px)
                    .setOuterPadding(8.f),
                panelRow,
                contains {
                    waveform(
                        Modifier()
                            .setWidth(100_pct)
                            .setHeight(100_pct),
                        WaveformOptions()
                            .setColorRole("text")
                            .setLeftChannelColorRole("accent.green")
                            .setRightChannelColorRole("accent.red")
                            .setBackgroundColorRole("waveform.bg")
                            .setRounding(ROUNDING - 2.f)
                            .setLineThickness(1.f)
                            .setLayout(WaveformLayout::Stacked)
                            .setGain(0.8f)
                            .setStyle(WaveformStyle::Line)
                            .setResolution(1.f),
                        "main_waveform"
                    ),
                }
            ),
        }, "root"
    );
}