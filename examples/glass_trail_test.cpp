// Regression test for the moving-glass trail: a Material::Blur panel sweeps
// across the window with children drawn under a round-clip in its glass
// subtree (the way Row/Column render a Material element's children). If the
// composite blit ever inherits the child's stale clip uniforms, only that
// region of the backbuffer is rewritten each frame and old panel positions
// ghost permanently. Correct output: exactly one panel visible, no trail.
#include "../include/renderer/Renderer.hpp"
#include <SDL3/SDL.h>
#include <cstdio>

using namespace uilo;

int main(int argc, char** argv) {
    const int frames = argc > 1 ? 100000 : 240;

    Renderer renderer;
    if (!renderer.init(800, 600, "glass_trail_test", 8)) return 1;

    for (int frame = 0; frame < frames; ++frame) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) return 0;
        }

        renderer.beginFrame();
        renderer.clear(Color{30, 31, 40, 255});

        // Static scene content: a few flat rects (batched path).
        renderer.draw(Rect{{ 50.f, 400.f}, {300.f, 120.f}, Color{60, 120, 200, 255}});
        renderer.draw(Rect{{400.f, 400.f}, {300.f, 120.f}, Color{200, 120, 60, 255}});

        // Moving glass panel, diagonal sweep like a drag — with children
        // rendered into the glass subtree (view kGlassChildViewId), the way
        // Row/Column render a Material element's children.
        const float t = (float)(frame % 180) / 180.f;
        Rectf dst{{40.f + 500.f * t, 40.f + 350.f * t}, {200.f, 100.f}};
        renderer.drawGlass(dst, Material::Blur().setRadius(2.f),
                           Color{60, 60, 90, 180});
        renderer.beginGlassSubtree();
        renderer.pushRoundClip(dst, 10.f);
        renderer.draw(Rect{{dst.position.x + 10.f, dst.position.y + 10.f},
                           {60.f, 20.f}, Color{220, 220, 240, 255}});
        renderer.draw(Triangle{{dst.position.x + 10.f, dst.position.y + 50.f},
                               {dst.position.x + 70.f, dst.position.y + 50.f},
                               {dst.position.x + 40.f, dst.position.y + 30.f},
                               Color{220, 220, 240, 255}});
        renderer.popRoundClip();
        renderer.endGlassSubtree();

        renderer.endFrame();
    }
    return 0;
}
