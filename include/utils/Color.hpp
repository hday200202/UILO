#pragma once

#include <cstdint>
#include <string_view>

namespace uilo {

struct Color {
    uint8_t r = 0, g = 0, b = 0, a = 255;

    constexpr Color() = default;
    constexpr Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
        : r(r), g(g), b(b), a(a) {}

    // Normalised [0,1] for GPU upload
    constexpr float rf() const { return r / 255.f; }
    constexpr float gf() const { return g / 255.f; }
    constexpr float bf() const { return b / 255.f; }
    constexpr float af() const { return a / 255.f; }

    // Pack to / unpack from 0xRRGGBBAA
    constexpr uint32_t toRGBA() const {
        return (uint32_t(r) << 24) | (uint32_t(g) << 16) |
               (uint32_t(b) <<  8) |  uint32_t(a);
    }
    static constexpr Color fromRGBA(uint32_t v) {
        return { uint8_t(v >> 24), uint8_t(v >> 16),
                 uint8_t(v >>  8), uint8_t(v) };
    }

    // Parse "#RRGGBB" or "#RRGGBBAA" (leading # optional)
    static Color fromHex(std::string_view hex);

    // Invert RGB, preserve alpha
    constexpr void invert() {
        r = uint8_t(255 - r);
        g = uint8_t(255 - g);
        b = uint8_t(255 - b);
    }

    // Return a copy with alpha multiplied by factor [0,1]
    constexpr Color withAlpha(float factor) const {
        return { r, g, b, uint8_t(a * factor) };
    }

    constexpr bool operator==(const Color& o) const {
        return r == o.r && g == o.g && b == o.b && a == o.a;
    }
    constexpr bool operator!=(const Color& o) const { return !(*this == o); }

    static const Color White;
    static const Color Black;
    static const Color Transparent;
    static const Color Red;
    static const Color Green;
    static const Color Blue;
};

inline constexpr Color Color::White       {255, 255, 255, 255};
inline constexpr Color Color::Black       {  0,   0,   0, 255};
inline constexpr Color Color::Transparent {  0,   0,   0,   0};
inline constexpr Color Color::Red         {255,   0,   0, 255};
inline constexpr Color Color::Green       {  0, 255,   0, 255};
inline constexpr Color Color::Blue        {  0,   0, 255, 255};

} // namespace uilo