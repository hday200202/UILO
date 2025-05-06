#include "UILO.hpp"

using namespace uilo;

sf::Color red = sf::Color::Red;
sf::Color green = sf::Color::Green;
sf::Color blue = sf::Color::Blue;

int main() {
    UILO ui ("My UI", {
        {new Page({
            {new Column(
                Modifier().setColor(sf::Color(25, 25, 45, 255)).setfixedWidth(100).align(Align::RIGHT), {

                {new Row(Modifier()
                    .setColor(red).setfixedHeight(50).align(Align::TOP), {
                    
                })},

                {new Row(Modifier()
                    .setColor(green).setfixedHeight(50).align(Align::BOTTOM), {
                    
                })},

                {new Row(Modifier()
                    .setColor(blue).setfixedHeight(50).align(Align::BOTTOM), {
                    
                })},
            })}
        }), "myPage"}
    });

    while (ui.isRunning()) {
        ui.update();
        ui.render();
    }

    return 0;
}
