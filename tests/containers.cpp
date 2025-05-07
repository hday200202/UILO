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

    Row* myRow = new Row(Modifier()
        .align(Align::TOP).setfixedHeight(100).setColor(sf::Color(25, 25, 45, 255)), {

        {new Column(Modifier()
            .setColor(red).align(Align::CENTER_X).setfixedWidth(100).onClick(testRedColumn), {

        })},

        {new Column(Modifier()
            .setColor(green).align(Align::CENTER_X).setfixedWidth(100).onClick(testGreenColumn), {

        })},

        {new Column(Modifier()
            .setColor(blue).align(Align::CENTER_X).setfixedWidth(100).onClick(testBlueColumn), {

        })},

        {new Column(Modifier()
            .setColor(white).align(Align::CENTER_X).setfixedWidth(100).onClick(testWhiteColumn), {

        })},
    });

    UILO ui ("My UI", {
        {new Page({
            {new Row(Modifier().setColor(sf::Color(25, 25, 45, 255)), {

                {new Column(Modifier()
                    .setColor(sf::Color(25, 25, 45, 255)).setfixedWidth(100).align(Align::LEFT), {

                    {new Row(Modifier()
                        .setColor(sf::Color::Black).align(Align::TOP).setfixedHeight(100).onClick([&]() {myRow->m_modifier.setVisible(!myRow->m_modifier.isVisible());}), {

                    })},

                    {new Row(Modifier()
                        .setColor(red).setfixedHeight(100).align(Align::CENTER_Y).onClick(testRedRow), {
                        
                    })},

                    {new Row(Modifier()
                        .setColor(green).setfixedHeight(100).align(Align::CENTER_Y).onClick(testGreenRow), {
                        
                    })},

                    {new Row(Modifier()
                        .setColor(blue).setfixedHeight(100).align(Align::CENTER_Y).onClick(testBlueRow), {
                        
                    })},

                    {new Row(Modifier()
                        .setColor(white).setfixedHeight(100).align(Align::CENTER_Y).onClick(testWhiteRow), {
                        
                    })},
                })},

                {new Column(Modifier().setfixedWidth(4).setColor(sf::Color(40, 40, 60, 255)), {})},

                myRow,
            })}
        }), "myPage"}
    });

    Page* myPage = new Page({
        {new Row(Modifier(), {

            {new Column(Modifier()
                .setColor(sf::Color(25, 25, 45, 255)).setfixedWidth(100).align(Align::LEFT), {

                {new Row(Modifier()
                    .setColor(sf::Color::Black).align(Align::TOP).setfixedHeight(100).onClick([&]() { ui.switchToPage("myPage"); }), {

                })},

                {new Row(Modifier()
                    .setColor(red).setfixedHeight(100).align(Align::CENTER_Y).onClick(testRedRow), {
                    
                })},

                {new Row(Modifier()
                    .setColor(green).setfixedHeight(100).align(Align::CENTER_Y).onClick(testGreenRow), {
                    
                })},

                {new Row(Modifier()
                    .setColor(blue).setfixedHeight(100).align(Align::CENTER_Y).onClick(testBlueRow), {
                    
                })},

                {new Row(Modifier()
                    .setColor(white).setfixedHeight(100).align(Align::CENTER_Y).onClick(testWhiteRow), {
                    
                })},
            })},
        })},
    });

    ui.addPage({myPage, "myPage2"});

    // UILO ui("MY UI", {
    //     {new Page({
    //         {new Row(Modifier().setColor(sf::Color::Cyan), {
    //             {new Column(Modifier().setColor(sf::Color::Red).setfixedWidth(100).align(Align::RIGHT), {

    //             })},

    //             {new Column(Modifier().setColor(sf::Color::Blue).setfixedWidth(100).align(Align::RIGHT), {

    //             })},

    //             {new Row(Modifier().setColor(green).align(Align::RIGHT), {
    //                 {new Column(Modifier().setColor(white).setfixedWidth(100).align(Align::RIGHT), {

    //                 })},
    //             })}
    //         })}
    //     }), "My Page"}
    // });

    while (ui.isRunning()) {
        ui.update();
        ui.render();
    }

    return 0;
}
