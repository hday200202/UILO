#pragma once

#include <iostream>
#include "UILO.hpp"

using namespace uilo;

inline Page* createMainPage(const uint8_t* imgPixels, uint32_t imgW, uint32_t imgH) {
    auto* header = row(
        Modifier()
            .setHeight(60_px)
            .setColor(Colors::Red)
            .setRounded(16.f)
            .setPadding(4.f),
        contains{}
    );
    auto* leftPanel = column(
        Modifier()
            .setWidth(100_pct)
            .setColor(Colors::Blue)
            .setRounded(16.f)
            .setPadding(4.f)
            .setAlign(Align::LEFT | Align::TOP),
        contains{
            text(
                Modifier()
                    .setWidth(100_pct)
                    .setHeight(100_pct)
                    .setAlign(Align::TOP | Align::LEFT)
                    .setColor(Colors::White), 
                18, 
                "Hello from the left panel!\nThis text should be clipped\nby the rounded corners\nof its parent container."
            ),
        }, "leftPanel"
    );
    auto* rightPanel = column(
        Modifier()
            .setWidth(100_pct)
            .setColor(Colors::Green)
            .setRounded(16.f)
            .setPadding(4.f)
            .setAlign(Align::RIGHT | Align::TOP),
        contains{
            image(
                Modifier()
                    .setWidth(256_px)
                    .setHeight(256_px),
                imgPixels, imgW, imgH
            ),
        }
    );
    auto* content = row(
        Modifier()
            .setHeight(100_pct)
            .setColor({30, 30, 30, 255}),
        contains{leftPanel, rightPanel}
    );
    auto* footer = row(
        Modifier()
            .setHeight(40_px)
            .setColor(Colors::Cyan)
            .setRounded(16.f)
            .setPadding(4.f)
            .setOnLeftClick([&](){ std::cout << "Footer Clicked" << std::endl; }),
        contains{}
    );
    auto* root = column(
        Modifier()
            .setWidth(100_pct)
            .setHeight(100_pct)
            .setColor({25, 25, 25, 255}),
        contains{header, content, footer}
    );

    return page(root, "main");
}