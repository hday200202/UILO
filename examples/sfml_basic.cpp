#include <SFML/Graphics.hpp>

#include "UILO.hpp"
#include "Factory.hpp"

using namespace uilo;

int main() {
    const unsigned int W = 800, H = 600;
    sf::RenderWindow window(sf::VideoMode({W, H}), "UILO - sfml_basic");
    window.setFramerateLimit(60);

    // Wire renderer callbacks to SFML draw calls
    Renderer renderer;
    renderer.setDrawFilledRect([&](Rect* r) {
        sf::RectangleShape shape({r->getSize().x, r->getSize().y});
        shape.setPosition({r->getPosition().x, r->getPosition().y});
        auto c = r->getColor();
        shape.setFillColor(sf::Color(c.r, c.g, c.b, c.a));
        window.draw(shape);
    });
    renderer.setDrawRoundedRect([&](Rect* r, float /*radius*/) {
        // TODO: proper rounded rect; fallback to filled for now
        sf::RectangleShape shape({r->getSize().x, r->getSize().y});
        shape.setPosition({r->getPosition().x, r->getPosition().y});
        auto c = r->getColor();
        shape.setFillColor(sf::Color(c.r, c.g, c.b, c.a));
        window.draw(shape);
    });

    // -------------------------------------------------------
    // Layout:
    //
    //   Column (full screen)
    //   ┌─────────────────────────────┐
    //   │  header  Row  (60px, red)   │
    //   ├──────────────┬──────────────┤
    //   │  left Col    │  right Col   │
    //   │  (50%, blue) │  (50%,green) │
    //   ├──────────────┴──────────────┤
    //   │  footer  Row  (40px, cyan)  │
    //   └─────────────────────────────┘
    // -------------------------------------------------------

    auto* header = row(
        Modifier().setHeight(60_px).setColor(Colors::Red),
        {}
    );

    auto* leftPanel = column(
        Modifier().setWidth(50_pct).setColor(Colors::Blue),
        {}
    );
    auto* rightPanel = column(
        Modifier().setWidth(50_pct).setColor(Colors::Green),
        {}
    );
    auto* content = row(
        Modifier().setHeight(100_pct).setColor({30, 30, 30, 255}),
        {leftPanel, rightPanel}
    );

    auto* footer = row(
        Modifier().setHeight(40_px).setColor(Colors::Cyan),
        {}
    );

    auto* root = column(
        Modifier().setWidth(100_pct).setHeight(100_pct).setColor(Colors::Transparent),
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

    return 0;
}

