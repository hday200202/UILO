#define UILO_SDL
#include "UILO.hpp"
#include <SDL3_image/SDL_image.h>
#include <iostream>

using namespace uilo;

int main() {
    SDL_Init(SDL_INIT_VIDEO);

    SDLRenderer::setGLAttributes();

    const int W = 1280, H = 720;
    SDL_Window* window = SDL_CreateWindow("UILO - sdl_basic", W, H,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    SDL_GLContext ctx = SDL_GL_CreateContext(window);

    SDLRenderer renderer(window, ctx);

    // Load settings icon
    SDL_Surface* imgSurf = IMG_Load("settings.jpeg");
    SDL_Surface* imgRGBA = SDL_ConvertSurface(imgSurf, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(imgSurf);
    const uint8_t* imgPixels = (const uint8_t*)imgRGBA->pixels;
    uint32_t imgW = imgRGBA->w;
    uint32_t imgH = imgRGBA->h;

    auto* header = row(
        Modifier()
            .setHeight(60_px)
            .setColor(Colors::Red)
            .setRounded(16.f)
            .setPadding(4.f),
        contains{}
    );
    auto* leftPanel = column(
        Modifier()
            .setWidth(100_pct)
            .setColor(Colors::Blue)
            .setRounded(16.f)
            .setPadding(4.f)
            .setAlign(Align::LEFT | Align::TOP),
        contains{
            text(
                Modifier()
                    .setWidth(100_pct)
                    .setHeight(100_pct)
                    .setAlign(Align::TOP | Align::LEFT)
                    .setColor(Colors::White), 
                18, 
                "Hello from the left panel!\nThis text should be clipped\nby the rounded corners\nof its parent container."
            ),
        }, "leftPanel"
    );
    auto* rightPanel = column(
        Modifier()
            .setWidth(100_pct)
            .setColor(Colors::Green)
            .setRounded(16.f)
            .setPadding(4.f)
            .setAlign(Align::RIGHT | Align::TOP),
        contains{
            image(
                Modifier()
                    .setWidth(256_px)
                    .setHeight(256_px),
                imgPixels, imgW, imgH
            ),
        }
    );
    auto* content = row(
        Modifier()
            .setHeight(100_pct)
            .setColor({30, 30, 30, 255}),
        contains{leftPanel, rightPanel}
    );
    auto* footer = row(
        Modifier()
            .setHeight(40_px)
            .setColor(Colors::Cyan)
            .setRounded(16.f)
            .setPadding(4.f)
            .setOnLeftClick([&](){ std::cout << "Footer Clicked" << std::endl; }),
        contains{}
    );
    auto* root = column(
        Modifier()
            .setWidth(100_pct)
            .setHeight(100_pct)
            .setColor({25, 25, 25, 255}),
        contains{header, content, footer}
    );

    UILO uilo;
    uilo.setScreenBounds({{0.f, 0.f}, {(float)W, (float)H}});
    uilo.addPage(page(root, "main"));
    uilo.setPage("main");

    Input input;
    bool running = true;

    while (running) {
        input.reset();

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT || e.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
                running = false;
            if (e.type == SDL_EVENT_WINDOW_RESIZED)
                uilo.setScreenBounds({{0.f, 0.f}, {(float)e.window.data1, (float)e.window.data2}});
            if (e.type == SDL_EVENT_KEY_DOWN) {
                if (e.key.key == SDLK_D)
                    if (auto* el = uilo.getElement<Column>("leftPanel"))
                        el->erase();
                if (e.key.key == SDLK_MINUS) {
                    uilo.setScale(std::min(4.0f, uilo.getScale() + 0.25f));
                    std::cout << "Scale: " << uilo.getScale() << std::endl;
                }
                if (e.key.key == SDLK_EQUALS) {
                    uilo.setScale(std::max(0.25f, uilo.getScale() - 0.25f));
                    std::cout << "Scale: " << uilo.getScale() << std::endl;
                }
                if (e.key.key == SDLK_RIGHT) {
                    leftPanel->getModifier().setWidth({leftPanel->getModifier().getWidth().value + 5, true});
                    rightPanel->getModifier().setWidth({rightPanel->getModifier().getWidth().value - 5, true});
                }
                if (e.key.key == SDLK_LEFT) {
                    leftPanel->getModifier().setWidth({leftPanel->getModifier().getWidth().value - 5, true});
                    rightPanel->getModifier().setWidth({rightPanel->getModifier().getWidth().value + 5, true});
                }
            }
            if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                if (e.button.button == SDL_BUTTON_LEFT)  input.leftMouse  = true;
                if (e.button.button == SDL_BUTTON_RIGHT) input.rightMouse = true;
            }
            if (e.type == SDL_EVENT_MOUSE_WHEEL)
                input.scrollDelta = e.wheel.y;
        }

        float mx, my;
        SDL_GetMouseState(&mx, &my);
        input.mousePosition = {mx, my};

        uilo.update(input);

        glClearColor(20.f/255.f, 20.f/255.f, 20.f/255.f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);
        uilo.render(renderer);
        SDL_GL_SwapWindow(window);
    }

    SDL_GL_DestroyContext(ctx);
    SDL_DestroySurface(imgRGBA);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
