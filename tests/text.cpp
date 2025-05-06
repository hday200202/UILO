#include <SFML/Graphics.hpp>
#include "UILO.hpp"

using namespace uilo;

int main() {
    sf::RenderWindow window(sf::VideoMode({800, 600}), "Uilo UI Framework");
    window.setFramerateLimit(60);

    View view(window, {
            new Column(Modifier().setColor(sf::Color::White).setfixedWidth(200.f).align(Align::CENTER_X), {
                new Element()
            }),

            new Row(Modifier().setColor(sf::Color::Blue).setfixedHeight(200.f).align(Align::CENTER_Y), {
                new Element()
            })
        }
    );

    while (window.isOpen()) {
        while (const auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>())
                window.close();
        }

        view.update(window);

        window.clear(sf::Color::Black);
        view.render(window);
        window.display();
    }

    return 0;
}
