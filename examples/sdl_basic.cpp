#define UILO_SDL
#include "UILO.hpp"
#include <SDL3_image/SDL_image.h>
#include <iostream>
#include "UI.hpp"

using namespace uilo;

int main() {
    SDL_Init(SDL_INIT_VIDEO);

    SDLRenderer::setGLAttributes();

    const int W = 1280, H = 720;
    SDL_Window* window = SDL_CreateWindow("UILO - sdl_basic", W, H,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    SDL_GLContext ctx = SDL_GL_CreateContext(window);
    SDL_GL_SetSwapInterval(1);

    SDLRenderer renderer(window, ctx);

    // Load settings icon
    SDL_Surface* imgSurf = IMG_Load("settings.jpeg");
    SDL_Surface* imgRGBA = SDL_ConvertSurface(imgSurf, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(imgSurf);

    const uint8_t* imgPixels = (const uint8_t*)imgRGBA->pixels;
    uint32_t imgW = imgRGBA->w;
    uint32_t imgH = imgRGBA->h;

    UILO uilo;
    uilo.addPage(createMainPage(imgPixels, imgW, imgH));
    uilo.setPage("main");

    Vec2f mousePosWin = {0.f, 0.f};
    Vec2f mousePosMon = {0.f, 0.f};

    bool running = true;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT || e.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
                running = false;
            if (e.type == SDL_EVENT_MOUSE_WHEEL)
                renderer.feedScrollDelta(e.wheel.y);
        }

        uilo.update(renderer);

        glClearColor(20.f/255.f, 20.f/255.f, 20.f/255.f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);
        uilo.render(renderer);
        SDL_GL_SwapWindow(window);

        if (mousePosWin != Mouse::get().windowPosition() || mousePosMon != Mouse::get().monitorPosition()) {
            mousePosWin = Mouse::get().windowPosition();
            mousePosMon = Mouse::get().monitorPosition();
            system("clear");
            std::cout << " Window: " << (int)mousePosWin.x << "\t\t" << (int)mousePosWin.y << std::endl;
            std::cout << "Monitor: " << (int)mousePosMon.x << "\t\t" << (int)mousePosMon.y << std::endl;
        }
    }

    SDL_GL_DestroyContext(ctx);
    SDL_DestroySurface(imgRGBA);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
