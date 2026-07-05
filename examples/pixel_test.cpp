// Test for Image::getPixel / setPixel: reads texels from a known solid-red
// PNG, writes a green block, verifies the CPU-side roundtrip, and runs a few
// frames so the copy-on-write texture upload path (syncPixels) executes.
#include "../include/UILO.hpp"
#include "../include/renderer/Renderer.hpp"
#include <SDL3/SDL.h>
#include <cstdio>
#include <cstdlib>

using namespace uilo;

#define CHECK(cond) do { if (!(cond)) { \
    std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
    std::exit(1); } } while (0)

int main(int argc, char** argv) {
    if (argc < 2) { std::fprintf(stderr, "usage: pixel_test <image.png>\n"); return 1; }

    Renderer renderer;
    if (!renderer.init(400, 400, "UILO pixel test", 8)) {
        std::fprintf(stderr, "Failed to initialize renderer\n");
        return 1;
    }

    UILO ui;
    ui.setRenderer(renderer);

    Image* img = image(
        Modifier().setWidth(90_pct).setHeight(90_pct)
                  .setAlign(Align::CenterX | Align::CenterY),
        ImageOptions().setPath(argv[1]),
        "testImage");
    ui.addPage(page(column(Modifier(), ColumnOptions(), contains{ img }), "main"));
    ui.setPage("main");

    // Reads before the first frame: the CPU decode is lazy but independent
    // of GPU texture creation.
    Color c = img->getPixel(0, 0);
    CHECK(c.r == 255 && c.g == 0 && c.b == 0 && c.a == 255);
    c = img->getPixel(63, 63);
    CHECK(c.r == 255 && c.g == 0 && c.b == 0 && c.a == 255);

    // Out-of-bounds is transparent, not UB.
    c = img->getPixel(64, 0);
    CHECK(c.a == 0);
    c = img->getPixel(0, 1000000);
    CHECK(c.a == 0);

    // Write a green block and read it back.
    for (uint32_t y = 16; y < 48; ++y)
        for (uint32_t x = 16; x < 48; ++x)
            img->setPixel(x, y, Color{0, 255, 0, 255});
    img->setPixel(9999, 9999, Color{255, 255, 255, 255}); // ignored, no crash

    c = img->getPixel(32, 32);
    CHECK(c.r == 0 && c.g == 255 && c.b == 0 && c.a == 255);
    c = img->getPixel(0, 0);   // outside the block: still red
    CHECK(c.r == 255 && c.g == 0);

    // Run a few frames so syncPixels creates the private texture and
    // uploads the writes.
    for (int frame = 0; frame < 30; ++frame) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) ui.handleEvent(event);
        ui.update();
        renderer.beginFrame();
        renderer.clear(Color{20, 20, 20, 255});
        ui.render();
        renderer.endFrame();
    }

    // Post-upload: CPU buffer still consistent, more writes still land.
    img->setPixel(0, 0, Color{0, 0, 255, 255});
    c = img->getPixel(0, 0);
    CHECK(c.r == 0 && c.g == 0 && c.b == 255);

    // Pass any second argument to keep the window up (visual inspection).
    const int tailFrames = argc > 2 ? 100000 : 5;
    for (int frame = 0; frame < tailFrames; ++frame) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) ui.handleEvent(event);
        ui.update();
        renderer.beginFrame();
        renderer.clear(Color{20, 20, 20, 255});
        ui.render();
        renderer.endFrame();
    }

    std::puts("pixel_test: OK");
    return 0;
}
