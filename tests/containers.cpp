#include "UILO.hpp"
using namespace uilo;

int main() {
    auto makeCYSpacer = []() {
        return spacer(
            Modifier()
                .setfixedHeight(12.5f)
                .align(Align::CENTER_Y)
        );
    };

    auto makeBSpacer = []() {
        return spacer(
            Modifier()
                .setfixedHeight(12.5f)
                .align(Align::BOTTOM)
        );
    };

    auto makeTSpacer = []() {
        return spacer(
            Modifier()
                .setfixedHeight(12.5f)
                .align(Align::TOP)
        );
    };

    UILO ui("Container Layout Test", {{
        page({
            row(
                Modifier()
                    .setColor(sf::Color::Transparent)
                    .setHeight(1.f)
                    .setWidth(1.f),  // Make row full width
            contains{
                // RIGHT COLUMN
                column(
                    Modifier()
                        .align(Align::RIGHT)
                        .setWidth(1.f)  // ✅ FIXED WIDTH REQUIRED for Align::RIGHT
                        .setColor(sf::Color(240, 240, 240)),
                contains{

                    row(
                        Modifier()
                            .align(Align::BOTTOM)
                            .setColor(sf::Color(220, 220, 220))
                            .setfixedHeight(100),
                    contains{

                        button(
                            Modifier()
                                .align(Align::CENTER_X | Align::CENTER_Y)
                                .setfixedWidth(75)
                                .setfixedHeight(75)
                                .setColor(sf::Color::Red)
                                .onClick([](){ std::cout << "Red\n"; }),
                            ButtonStyle::Pill
                        ),

                        button(
                            Modifier()
                                .align(Align::CENTER_X | Align::CENTER_Y)
                                .setfixedWidth(75)
                                .setfixedHeight(75)
                                .setColor(sf::Color::Green)
                                .onClick([](){ std::cout << "Green\n"; }),
                            ButtonStyle::Pill
                        ),

                        button(
                            Modifier()
                                .align(Align::CENTER_X | Align::CENTER_Y)
                                .setfixedWidth(75)
                                .setfixedHeight(75)
                                .setColor(sf::Color::Blue)
                                .onClick([](){ std::cout << "Blue\n"; }),
                            ButtonStyle::Pill
                        ),

                        button(
                            Modifier()
                                .align(Align::CENTER_X | Align::CENTER_Y)
                                .setfixedWidth(75)
                                .setfixedHeight(75)
                                .setColor(sf::Color::Black)
                                .onClick([](){ std::cout << "Black\n"; }),
                            ButtonStyle::Pill
                        ),

                        button(
                            Modifier()
                                .align(Align::CENTER_X | Align::CENTER_Y)
                                .setfixedWidth(75)
                                .setfixedHeight(75)
                                .setColor(sf::Color::White)
                                .onClick([](){ std::cout << "White\n"; }),
                            ButtonStyle::Pill
                        )
                    })
                }),

                // LEFT COLUMN
                column(
                    Modifier()
                        .align(Align::LEFT)
                        .setfixedWidth(100)
                        .setColor(sf::Color::White),
                contains{

                    makeTSpacer(),

                    button(
                        Modifier()
                            .align(Align::CENTER_X | Align::CENTER_Y)
                            .setfixedWidth(75)
                            .setfixedHeight(75)
                            .setColor(sf::Color::Red),
                        ButtonStyle::Pill
                    ),

                    makeCYSpacer(),

                    button(
                        Modifier()
                            .align(Align::CENTER_X | Align::CENTER_Y)
                            .setfixedWidth(75)
                            .setfixedHeight(75)
                            .setColor(sf::Color::Red),
                        ButtonStyle::Pill
                    ),

                    makeCYSpacer(),

                    button(
                        Modifier()
                            .align(Align::CENTER_X | Align::CENTER_Y)
                            .setfixedWidth(75)
                            .setfixedHeight(75)
                            .setColor(sf::Color::Red),
                        ButtonStyle::Pill
                    ),

                    makeBSpacer(),

                    button(
                        Modifier()
                            .align(Align::CENTER_X | Align::BOTTOM)
                            .setfixedWidth(75)
                            .setfixedHeight(75)
                            .setColor(sf::Color::Red),
                        ButtonStyle::Pill
                    ),

                    makeBSpacer()
                }),
            })
        }), "test" }
    });

    while (ui.isRunning()) {
        ui.update();
        ui.render();
    }

    return 0;
}