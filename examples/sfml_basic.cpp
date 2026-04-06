#define UILO_SFML
#include "UILO.hpp"
#include <iostream>

using namespace uilo;

int main() {
    const unsigned int W = 1280, H = 720;
    sf::RenderWindow window(sf::VideoMode({W, H}), "UILO - sfml_basic", sf::Style::Default, sf::State::Windowed, SFMLRenderer::recommendedSettings());
    window.setFramerateLimit(60);

    SFMLRenderer renderer(window);

    auto* header = row(
        Modifier().setHeight(60_px).setColor(Colors::Red).setRounded(16.f).setPadding(4.f),
        {}
    );

    auto* leftPanel = column(
        Modifier().setWidth(50_pct).setColor(Colors::Blue).setRounded(16.f).setPadding(4.f),
        {
            text(Modifier().setWidth(100_pct).setHeight(100_pct).setAlign(Align::TOP | Align::LEFT).setColor(Colors::White), 18, "Hello from the left panel!\nThis text should be clipped\nby the rounded corners\nof its parent container."),
        }, "leftPanel"
    );
    auto* rightPanel = column(
        Modifier().setWidth(50_pct).setColor(Colors::Green).setRounded(16.f).setPadding(4.f),
        {
            row(Modifier().setWidth(100_pct).setHeight(100_pct).setAlign(Align::CENTER_X | Align::CENTER_Y), {}),
        }
    );
    auto* content = row(
        Modifier().setHeight(100_pct).setColor({30, 30, 30, 255}),
        {leftPanel, rightPanel}
    );
    auto* footer = row(
        Modifier().setHeight(40_px).setColor(Colors::Cyan).setRounded(16.f).setPadding(4.f)
            .setOnLeftClick([&](){ std::cout << "Footer Clicked" << std::endl; }),
        {}
    );
    auto* root = column(
        Modifier().setWidth(100_pct).setHeight(100_pct).setColor({25, 25, 25, 255}),
        {header, content, footer}
    );

    UILO uilo;
    uilo.setScreenBounds({{0.f, 0.f}, {(float)W, (float)H}});
    uilo.setOnResize([&](float w, float h) {
        window.setView(sf::View(sf::FloatRect({0.f, 0.f}, {w, h})));
    });
    uilo.addPage(page(root, "main"));
    uilo.setPage("main");

    Input input;

    while (window.isOpen()) {
        input.reset();

        while (auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>())
                window.close();
            if (auto* e = event->getIf<sf::Event::Resized>())
                uilo.setScreenBounds({{0.f, 0.f}, {(float)e->size.x, (float)e->size.y}});
            if (auto* e = event->getIf<sf::Event::KeyPressed>()) {
                if (e->code == sf::Keyboard::Key::D)
                    if (auto* el = uilo.getElement<Column>("leftPanel"))
                        el->erase();
                if (e->code == sf::Keyboard::Key::Hyphen) {
                    uilo.setScale(std::min(4.0f, uilo.getScale() + 0.25f));
                    std::cout << "Scale: " << uilo.getScale() << std::endl;
                }
                if (e->code == sf::Keyboard::Key::Equal) {
                    uilo.setScale(std::max(0.25f, uilo.getScale() - 0.25f));
                    std::cout << "Scale: " << uilo.getScale() << std::endl;
                }
            }
            if (auto* e = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (e->button == sf::Mouse::Button::Left)  input.leftMouse  = true;
                if (e->button == sf::Mouse::Button::Right) input.rightMouse = true;
            }
            if (auto* e = event->getIf<sf::Event::MouseWheelScrolled>())
                input.scrollDelta = e->delta;
        }

        auto mpos = sf::Mouse::getPosition(window);
        input.mousePosition = {(float)mpos.x, (float)mpos.y};

        uilo.update(input);

        window.clear(sf::Color(20, 20, 20));
        uilo.render(renderer);
        window.display();
    }
}

