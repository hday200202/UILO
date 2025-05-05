#include <SFML/Graphics.hpp>
#include "UILO.hpp"

int main() {
    sf::RenderWindow window(sf::VideoMode({800, 600}), "Uilo Text Test");
    window.setVerticalSyncEnabled(true);

    // Create the UI view, passing the render target
    uilo::View ui(&window);

    // Create the text element
    auto* helloText = new uilo::Text("Hello, UILO!", "assets/fonts/DepartureMono-Regular.otf", 36);
    helloText->setAlignment(uilo::Align::CENTER_H | uilo::Align::CENTER_V);

    // Optional: set fixed position instead of alignment if needed
    // helloText->setPosition(400, 300);

    // Add the text to the view
    ui.add(helloText);

    while (window.isOpen()) {
        while (const auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>())
                window.close();
        }

        ui.update();           // Apply fixed pixel view and update layout
        window.clear();
        ui.render();           // Render all UI elements
        window.display();
    }

    delete helloText; // Cleanup for now (or switch to smart pointers later)
    return 0;
}