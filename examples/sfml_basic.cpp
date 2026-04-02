#include <SFML/Graphics.hpp>
#include <iostream>

#include "UILO.hpp"
#include "Factory.hpp"

using namespace uilo;

int main() {
    const unsigned int W = 1280, H = 720;
    sf::ContextSettings settings;
    settings.antiAliasingLevel = 8;
    sf::RenderWindow window(sf::VideoMode({W, H}), "UILO - sfml_basic", sf::Style::Default, sf::State::Windowed, settings);
    window.setFramerateLimit(60);

    // Wire renderer callbacks to SFML draw calls
    Renderer renderer;

    auto drawRect = [&](Rect* r) {
        const auto& verts   = r->getVertices();
        const auto& indices = r->getIndices();
        sf::VertexArray va(sf::PrimitiveType::Triangles, indices.size());
        for (size_t i = 0; i < indices.size(); i++) {
            const auto& v = verts[indices[i]];
            va[i].position = {v.position.x, v.position.y};
            va[i].color    = sf::Color(v.color.r, v.color.g, v.color.b, v.color.a);
        }
        window.draw(va);
    };

    renderer.setDrawFilledRect([&](Rect* r) { drawRect(r); });
    renderer.setDrawRoundedRect([&](Rect* r, float radius) {
        r->setCornerRadius(radius); drawRect(r);
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
        Modifier().setHeight(60_px).setColor(Colors::Red).setRounded(16.f).setPadding(4.f),
        {}
    );

    auto* leftPanel = column(
        Modifier().setWidth(50_pct).setColor(Colors::Blue).setRounded(16.f).setPadding(4.f),
        {}, "leftPanel"
    );
    auto* rightPanel = column(
        Modifier().setWidth(50_pct).setColor(Colors::Green).setRounded(16.f).setPadding(4.f),
        {}
    );
    auto* content = row(
        Modifier().setHeight(100_pct).setColor({30, 30, 30, 255}),
        {leftPanel, rightPanel}
    );

    auto* footer = row(
        Modifier().setHeight(40_px).setColor(Colors::Cyan).setRounded(16.f).setPadding(4.f).setOnLeftClick([&](){std::cout << "Footer Clicked" << std::endl;}),
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
            if (auto* e = event->getIf<sf::Event::KeyPressed>())
                if (e->code == sf::Keyboard::Key::D)
                    if (auto* el = uilo.getElement<Column>("leftPanel"))
                        el->erase();
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

