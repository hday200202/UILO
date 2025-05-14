#include "UILO.hpp"
using namespace uilo;

sf::Color sideBarColor = sf::Color(57, 62, 70, 255);
sf::Color bottomBarColor = sf::Color(34, 40, 49, 255);
sf::Color backGroundColor = sf::Color(34, 40, 49, 255);
sf::Color buttonColor = sf::Color(148, 137, 121, 255);

int main() {
    std::string relativePath = "assets/fonts/BebasNeue-Regular.ttf";

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

    auto myButton = [&]() {
        return button(
            Modifier()
                .align(Align::CENTER_X | Align::CENTER_Y)
                .setfixedWidth(75)
                .setfixedHeight(75)
                .setColor(buttonColor)
                .onClick([](){ std::cout << "Blue\n"; }),
            ButtonStyle::Pill,
            "OFF",
            "assets/fonts/BebasNeue-Regular.ttf",
            sf::Color::White
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

                        myButton(),

                        makeCXSpacer(),

                        myButton(),

                        makeCXSpacer(),

                        myButton(),

                        makeCXSpacer(),

                        myButton(),

                        makeCXSpacer(),

                        myButton(),
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