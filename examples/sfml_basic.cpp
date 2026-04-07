#define UILO_SFML
#include "UILO.hpp"
#include "UI.hpp"
#include <iostream>

using namespace uilo;

int main() {
    const unsigned int W = 1280, H = 720;
    sf::RenderWindow window(sf::VideoMode({W, H}), "UILO - sfml_basic", sf::Style::Default, sf::State::Windowed, SFMLRenderer::recommendedSettings());
    window.setFramerateLimit(60);

    SFMLRenderer renderer(window);

    // Load settings icon
    sf::Image settingsImg;
    if (!settingsImg.loadFromFile("settings.jpeg")) return 1;
    auto imgSize = settingsImg.getSize();
    const uint8_t* imgPixels = settingsImg.getPixelsPtr();
    uint32_t imgW = imgSize.x; uint32_t imgH = imgSize.y;

    UILO uilo;
    uilo.addPage(createMainPage(imgPixels, imgW, imgH));
    uilo.setPage("main");

    Vec2f mousePosWin = {0.f, 0.f};
    Vec2f mousePosMon = {0.f, 0.f};

    while (window.isOpen()) {
        while (auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>())
                window.close();
            if (auto* e = event->getIf<sf::Event::Resized>())
                window.setView(sf::View(sf::FloatRect({0.f, 0.f}, {(float)e->size.x, (float)e->size.y})));
            if (auto* e = event->getIf<sf::Event::MouseWheelScrolled>())
                renderer.feedScrollDelta(e->delta);
        }

        uilo.update(renderer);

        window.clear(sf::Color(20, 20, 20));
        uilo.render(renderer);
        window.display();

        if (mousePosWin != Mouse::get().windowPosition() || mousePosMon != Mouse::get().monitorPosition()) {
            mousePosWin = Mouse::get().windowPosition();
            mousePosMon = Mouse::get().monitorPosition();
            system("clear");
            std::cout << " Window: " << (int)mousePosWin.x << "\t\t" << (int)mousePosWin.y << std::endl;
            std::cout << "Monitor: " << (int)mousePosMon.x << "\t\t" << (int)mousePosMon.y << std::endl;
        }
    }
}

