#include "UILO.hpp"

using namespace uilo;

sf::Color red = sf::Color::Red;
sf::Color green = sf::Color::Green;
sf::Color blue = sf::Color::Blue;
sf::Color white = sf::Color::White;

void testRedColumn() { std::cout << "Red Column" << std::endl; }
void testGreenColumn() { std::cout << "Green Column" << std::endl; }
void testBlueColumn() { std::cout << "Blue Column" << std::endl; }
void testWhiteColumn() { std::cout << "White Column" << std::endl; }
void testRedRow() { std::cout << "Red Row" << std::endl; }
void testGreenRow() { std::cout << "Green Row" << std::endl; }
void testBlueRow() { std::cout << "Blue Row" << std::endl; }
void testWhiteRow() { std::cout << "White Row" << std::endl; }
void testBlackRow() { std::cout << "Black Row" << std::endl; }

int main() {

    Row* myRow = new Row
    (
        Modifier()
        .align(Align::TOP)
        .setfixedHeight(100)
        .setColor(sf::Color(25, 25, 45, 255)), 
    {
        
        new Column
        (
            Modifier()
            .setColor(red)
            .align(Align::CENTER_X)
            .setfixedWidth(100)
            .onClick(testRedColumn), 
        {
            // Column Contents
            // ...
        }),
        
        new Column
        (
            Modifier()
            .setColor(green)
            .align(Align::CENTER_X)
            .setfixedWidth(100)
            .onClick(testGreenColumn), 
        {
            // Column Contents
            // ...
        }),

        new Column
        (
            Modifier()
            .setColor(blue)
            .align(Align::CENTER_X)
            .setfixedWidth(100)
            .onClick(testBlueColumn), 
        {
            // Column Contents
            // ...
        }),

        new Column
        (
            Modifier()
            .setColor(white)
            .align(Align::CENTER_X)
            .setfixedWidth(100)
            .onClick(testWhiteColumn), 
        {
            // Column Contents
            // ...
        }),
    });

    UILO ui ("My UI", {{
        new Page ({
            new Row
            (
                Modifier()
                .setColor(sf::Color(25, 25, 45, 255)), 
            {

                new Column
                (
                    Modifier()
                    .setColor(sf::Color(25, 25, 45, 255))
                    .setfixedWidth(100)
                    .align(Align::LEFT), 
                {

                    new Row
                    (
                        Modifier()
                        .setColor(sf::Color::Black)
                        .align(Align::TOP)
                        .setfixedHeight(100)
                        .onClick([&](){ui.switchToPage("myPage2");}), 
                    {
                        // Row Contents
                        // ...
                    }),

                    new Row
                    (
                        Modifier()
                        .setColor(red)
                        .setfixedHeight(100)
                        .align(Align::CENTER_Y)
                        .onClick(testRedRow), 
                    {
                        // Row Contents
                        // ...
                    }),

                    new Row
                    (
                        Modifier()
                        .setColor(green)
                        .setfixedHeight(100)
                        .align(Align::CENTER_Y)
                        .onClick(testGreenRow), 
                    {
                        // Row Contents
                        // ...
                    }),

                    new Row
                    (
                        Modifier()
                        .setColor(blue)
                        .setfixedHeight(100)
                        .align(Align::CENTER_Y)
                        .onClick(testBlueRow), 
                    {
                        // Row Contents
                        // ...
                    }),

                    new Row
                    (
                        Modifier()
                        .setColor(white)
                        .setfixedHeight(100)
                        .align(Align::CENTER_Y)
                        .onClick(testWhiteRow), 
                    {
                        // Row Contents
                        // ...
                    }),
                }),

                new Column
                (
                    Modifier()
                    .setfixedWidth(4)
                    .setColor(sf::Color(40, 40, 60, 255)), 
                {
                    // Column Contents
                    // ...
                }),

                myRow,
            })
        }), "myPage"}
    });

    Page* myPage2 = new Page({
        new Row(Modifier(), {

            new Column(Modifier()
                .setColor(sf::Color(25, 25, 45, 255))
                .setfixedWidth(100)
                .align(Align::LEFT), 
                {

                new Row(Modifier()
                    .setColor(sf::Color::Black)
                    .align(Align::TOP)
                    .setfixedHeight(100)
                    .onClick([&]() { ui.switchToPage("myPage"); }), 
                {
                    // Row Contents
                    // ...
                }),

                new Row(Modifier()
                    .setColor(red)
                    .setfixedHeight(100)
                    .align(Align::CENTER_Y)
                    .onClick(testRedRow), 
                {
                    // Row Contents
                    // ...
                }),

                new Row(Modifier()
                    .setColor(green)
                    .setfixedHeight(100)
                    .align(Align::CENTER_Y)
                    .onClick(testGreenRow), 
                {
                    // Row Contents
                    // ...
                }),

                new Row(Modifier()
                    .setColor(blue)
                    .setfixedHeight(100)
                    .align(Align::CENTER_Y)
                    .onClick(testBlueRow), 
                {
                    // Row Contents
                    // ...
                }),

                new Row(Modifier()
                    .setColor(white)
                    .setfixedHeight(100)
                    .align(Align::CENTER_Y)
                    .onClick(testWhiteRow), 
                {
                    // Row Contents
                    // ...
                }),
            }),
        }),
    });

    ui.addPage({myPage2, "myPage2"});

    // delete myRow;
    // delete myPage2;

    while (ui.isRunning()) {
        ui.update();
        ui.render();
    }

    return 0;
}