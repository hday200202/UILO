#include "UILO.hpp"
using namespace uilo;

sf::Color sideBarColor = sf::Color(57, 62, 70, 255);
sf::Color bottomBarColor = sf::Color(34, 40, 49, 255);
sf::Color backGroundColor = sf::Color(34, 40, 49, 255);
sf::Color buttonColor = sf::Color(148, 137, 121, 255);

int main() {
    auto makeCXSpacer = []() {
        return spacer(
            Modifier()
                .setfixedWidth(12.5f)
                .align(Align::CENTER_X)
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
                    .setWidth(1.f),
            contains{
                column(
                    Modifier()
                        .align(Align::RIGHT)
                        .setWidth(1.f)
                        .setColor(backGroundColor),
                contains{

                    row(
                        Modifier()
                            .align(Align::BOTTOM)
                            .setColor(bottomBarColor)
                            .setfixedHeight(100),
                    contains{

                        button(
                            Modifier()
                                .align(Align::CENTER_X | Align::CENTER_Y)
                                .setfixedWidth(75)
                                .setfixedHeight(75)
                                .setColor(buttonColor)
                                .onClick([](){ std::cout << "Red\n"; }),
                            ButtonStyle::Pill
                        ),

                        makeCXSpacer(),

                        button(
                            Modifier()
                                .align(Align::CENTER_X | Align::CENTER_Y)
                                .setfixedWidth(75)
                                .setfixedHeight(75)
                                .setColor(buttonColor)
                                .onClick([](){ std::cout << "Green\n"; }),
                            ButtonStyle::Pill
                        ),

                        makeCXSpacer(),

                        button(
                            Modifier()
                                .align(Align::CENTER_X | Align::CENTER_Y)
                                .setfixedWidth(75)
                                .setfixedHeight(75)
                                .setColor(buttonColor)
                                .onClick([](){ std::cout << "Blue\n"; }),
                            ButtonStyle::Pill
                        ),

                        makeCXSpacer(),

                        button(
                            Modifier()
                                .align(Align::CENTER_X | Align::CENTER_Y)
                                .setfixedWidth(75)
                                .setfixedHeight(75)
                                .setColor(buttonColor)
                                .onClick([](){ std::cout << "Black\n"; }),
                            ButtonStyle::Pill
                        ),

                        makeCXSpacer(),

                        button(
                            Modifier()
                                .align(Align::CENTER_X | Align::CENTER_Y)
                                .setfixedWidth(75)
                                .setfixedHeight(75)
                                .setColor(buttonColor)
                                .onClick([](){ std::cout << "White\n"; }),
                            ButtonStyle::Pill
                        )
                    })
                }),

                column(
                    Modifier()
                        .align(Align::LEFT)
                        .setfixedWidth(100)
                        .setColor(sideBarColor),
                contains{

                    makeTSpacer(),

                    button(
                        Modifier()
                            .align(Align::CENTER_X | Align::TOP)
                            .setfixedWidth(75)
                            .setfixedHeight(75)
                            .setColor(buttonColor),
                        ButtonStyle::Pill
                    ),

                    makeTSpacer(),

                    button(
                        Modifier()
                            .align(Align::CENTER_X | Align::TOP)
                            .setfixedWidth(75)
                            .setfixedHeight(75)
                            .setColor(buttonColor),
                        ButtonStyle::Pill
                    ),

                    makeTSpacer(),

                    button(
                        Modifier()
                            .align(Align::CENTER_X | Align::TOP)
                            .setfixedWidth(75)
                            .setfixedHeight(75)
                            .setColor(buttonColor),
                        ButtonStyle::Pill
                    ),
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