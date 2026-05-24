#include "../include/UILO.hpp"
#include <iostream>

using namespace uilo;

int main() {
    sf::RenderWindow window;
    sf::VideoMode screenRes({1920, 1080});

    sf::ContextSettings settings;
    settings.antiAliasingLevel = 8;

    window.create(
        screenRes, 
        "Containers", 
        sf::Style::Default, 
        sf::State::Windowed,
        settings
    );
    
    window.setVerticalSyncEnabled(true);

    // rgb(44, 47, 60)
    sf::Color bgColor       = {33, 35, 47};
    sf::Color contColor     = {44, 47, 60};

    float rounding = 8.f;

    UILO ui(
        window, 
        page(
            column(
                Modifier(), 
                ColumnOptions().setColor(bgColor), 
                contains {
                    row(
                        Modifier()
                            .setHeight(96_px)
                            .setOuterPadding(8.f),
                        RowOptions().setColor(contColor).setRounding(rounding),
                        contains {}
                    ),
                    
                    row(
                        Modifier(),
                        RowOptions(),
                        contains {

                            column(
                                Modifier()
                                    .setOuterPadding(8.f)
                                    .setWidth(35_pct),
                                ColumnOptions().setColor(contColor).setRounding(rounding),
                                contains {
                                    image(
                                        Modifier()
                                            .setWidth(512_px)
                                            .setHeight(512_px)
                                            .setAlign(Align::Top | Align::Left),
                                        ImageOptions()
                                            .setPath("assets/images/stones.jpg"),
                                        "stones"
                                    )
                                }, "1"
                            ),

                            column(
                                Modifier()
                                    .setOuterPadding(8.f),
                                ColumnOptions().setColor(contColor).setRounding(rounding),
                                contains{
                                    button(
                                        Modifier()
                                            .setAlign(Align::CenterX | Align::CenterY)
                                            .setWidth(192_px)
                                            .setHeight(64_px)
                                            .setOnLeftClick([&](){ std::cout << "Test button clicked!!!" << std::endl; }),
                                        ButtonOptions()
                                            .setColor(sf::Color::Blue)
                                            .setRounding(rounding)
                                            .setLabel(
                                                text(
                                                    Modifier()
                                                        .setAlign(Align::CenterX | Align::CenterY),
                                                    TextOptions()
                                                        .setFont("assets/fonts/Montserrat.ttf")
                                                        .setContent("TEST")
                                                        .setCharSize(36)
                                                        .setColor(sf::Color::White)
                                                        .setTextAlignX(Align::CenterX)
                                                        .setTextAlignY(Align::CenterY)
                                                )
                                            ),
                                        "test_button"
                                    )
                                }, "2"
                            )
                        }
                    ),

                    row(
                        Modifier()
                            .setHeight(256_px)
                            .setOuterPadding(8.f),
                        RowOptions().setColor(contColor).setRounding(rounding),
                        contains {
                            slider(
                                Modifier()
                                    .setHeight(32_px)
                                    .setAlign(Align::CenterX | Align::CenterY), 
                                SliderOptions()
                                    .setThumbShape(ThumbShape::Rect)
                                    .setThumbSize(16, 48)
                                    .setOnValueChanged([&](float v){std::cout << "Value to " << v << std::endl; })
                                    .setFillColor(sf::Color::Black)
                                    .setThumbRounding(8.f),
                                ""
                            )
                        }
                    ),
                }, "root"
            ), "main_page"
        )
    );

    sf::Font fpsFont;
    std::optional<sf::Text> fpsText;
    if (fpsFont.openFromFile("assets/fonts/Montserrat.ttf")) {
        fpsText.emplace(fpsFont, "FPS: 0", 18u);
        fpsText->setFillColor(sf::Color::White);
        fpsText->setOutlineColor(sf::Color::Black);
        fpsText->setOutlineThickness(1.f);
        fpsText->setPosition({8.f, 8.f});
    }

    bool showFps = false;

    float dt = 0.f;
    float timer = 0.f;

    float fpsSum = 0.f;
    int loopCount = 0.f;

    while (window.isOpen()) {
        while (const auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>())
                window.close();
            if (const auto* key = event->getIf<sf::Event::KeyPressed>())
                if (key->code == sf::Keyboard::Key::F10)
                    showFps = !showFps;
        }

        dt = ui.getDeltaTime();
        timer += dt;

        if (dt > 0.f) {
            fpsSum += 1.f / dt;
            loopCount++;
        }

        if (timer >= 1.f) {
            timer = 0.f;
            float fps = loopCount > 0 ? fpsSum / loopCount : 0.f;
            if (fpsText) fpsText->setString("FPS: " + std::to_string(static_cast<int>(fps)));
            fpsSum = 0.f;
            loopCount = 0;
        }

        ui.update();

        window.clear();
        ui.render();
        if (showFps && fpsText) window.draw(*fpsText);
        window.display();
    }
}