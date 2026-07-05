// Renderer stress bench: a 30x30 grid of flat colored cells (each cell is a
// Row, so the per-child pushRoundClip/popRoundClip churn in Container render
// is exercised 900+ times a frame) plus text labels. Prints bgfx draw-call
// count, CPU frame time, and average framerate so renderer changes can be
// compared before/after. Deterministic layout and colors; also used for
// pixel-identity screenshots.
//
// Usage: render_bench [vsync=true|false] [hold=true|false] [duration=<sec>]
//   vsync    - present with vsync (default true)
//   hold     - keep the window open indefinitely, e.g. for screenshots
//              (default false; bare "hold" also accepted)
//   duration - measurement length in seconds (default 5)
// Arguments may appear in any order.
#include "../include/UILO.hpp"
#include "../include/renderer/Renderer.hpp"
#include <SDL3/SDL.h>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string_view>

using namespace uilo;

int main(int argc, char** argv) {
    bool   vsync    = true;
    bool   hold     = false;
    double duration = 5.0;

    for (int i = 1; i < argc; ++i) {
        const std::string_view arg{argv[i]};
        const size_t eq = arg.find('=');
        if (eq == std::string_view::npos) {
            if (arg == "hold") { hold = true; continue; }
            std::fprintf(stderr, "unknown argument '%s'\n"
                "usage: render_bench [vsync=true|false] [hold=true|false] [duration=<sec>]\n",
                argv[i]);
            return 1;
        }
        const std::string_view key = arg.substr(0, eq);
        const std::string_view val = arg.substr(eq + 1);
        const bool truthy = val == "true" || val == "1";
        if      (key == "vsync")    vsync = truthy;
        else if (key == "hold")     hold  = truthy;
        else if (key == "duration") duration = std::atof(std::string(val).c_str());
        else {
            std::fprintf(stderr, "unknown argument '%s'\n"
                "usage: render_bench [vsync=true|false] [hold=true|false] [duration=<sec>]\n",
                argv[i]);
            return 1;
        }
    }
    if (duration <= 0.0) duration = 5.0;

    Renderer renderer;
    if (!renderer.init(1000, 700, "UILO render bench", 8)) {
        std::fprintf(stderr, "Failed to initialize renderer\n");
        return 1;
    }
    renderer.setVsync(vsync);

    UILO ui;
    ui.setRenderer(renderer);

    constexpr int kRows = 30, kCols = 30;
    Column* root = column(
        Modifier().setOuterPadding(4.f),
        ColumnOptions().setColor(Color{24, 25, 34, 255}));
    root->addElement(text(
        Modifier().setHeight(Dimension{24.f, false}),
        TextOptions().setContent("The quick brown fox jumps over the lazy dog 0123456789")
                     .setCharSize(18).setColor(Color::White)));
    root->addElement(text(
        Modifier().setHeight(Dimension{24.f, false}),
        TextOptions().setContent("UILO render bench - draw calls & CPU ms")
                     .setCharSize(18).setColor(Color{200, 220, 255, 255})));
    for (int r = 0; r < kRows; ++r) {
        Row* rowEl = row(Modifier().setHeight(Dimension{100.f / kRows, true})
                                   .setOuterPadding(1.f),
                         RowOptions());
        for (int c = 0; c < kCols; ++c) {
            const uint8_t cr = (uint8_t)(40 + (r * 7 + c * 13) % 180);
            const uint8_t cg = (uint8_t)(40 + (r * 11 + c * 5) % 180);
            const uint8_t cb = (uint8_t)(60 + (r * 3 + c * 17) % 160);
            rowEl->addElement(
                row(Modifier().setWidth(Dimension{100.f / kCols, true})
                              .setOuterPadding(1.f),
                    RowOptions().setColor(Color{cr, cg, cb, 255})));
        }
        root->addElement(rowEl);
    }

    ui.addPage(page(root, "main"));
    ui.setPage("main");

    using clock = std::chrono::steady_clock;
    constexpr int kWarmupFrames = 30;   // excluded from all stats

    const auto tStart = clock::now();
    auto tMeasureStart = tStart;
    double   cpuSum   = 0.0;
    uint32_t drawLast = 0;
    long     measured = 0;
    long     frame    = 0;

    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) running = false;
            ui.handleEvent(event);
        }
        ui.update();
        renderer.beginFrame();
        renderer.clear(Color{24, 25, 34, 255});
        ui.render();
        renderer.endFrame();

        ++frame;
        if (frame == kWarmupFrames) tMeasureStart = clock::now();
        if (frame > kWarmupFrames) {
            RendererStats st = renderer.getStats();
            cpuSum  += st.cpuTimeMs;
            drawLast = st.numDraw;
            ++measured;
        }

        if (!hold) {
            const double elapsed =
                std::chrono::duration<double>(clock::now() - tStart).count();
            if (elapsed >= duration) running = false;
        }
    }

    if (measured > 0) {
        const double measuredSec =
            std::chrono::duration<double>(clock::now() - tMeasureStart).count();
        const double avgFps = measuredSec > 0.0 ? (double)measured / measuredSec : 0.0;
        std::printf("render_bench: vsync=%s drawCalls=%u avgFps=%.1f avgCpuMs=%.3f frames=%ld (%.1fs)\n",
                    vsync ? "on" : "off", drawLast, avgFps,
                    cpuSum / (double)measured, measured, measuredSec);
    }
    return 0;
}
