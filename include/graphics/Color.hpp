#pragma once

#include <cstdint>

namespace uilo {

struct Color {
    uint8_t r = 255;
    uint8_t g = 255;
    uint8_t b = 255;
    uint8_t a = 255;

    Color(const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t a)
        : r(r), g(g), b(b), a(a) {}

    Color inverted() const {
        return {static_cast<uint8_t>(255 - r), static_cast<uint8_t>(255 - g),
                static_cast<uint8_t>(255 - b), a};
    }
};

namespace Colors {

inline const Color Black        = {  0,   0,   0, 255};
inline const Color White        = {255, 255, 255, 255};
inline const Color Red          = {255,   0,   0, 255};
inline const Color Green        = {  0, 255,   0, 255};
inline const Color Blue         = {  0,   0, 255, 255};
inline const Color Yellow       = {255, 255,   0, 255};
inline const Color Cyan         = {  0, 255, 255, 255};
inline const Color Magenta      = {255,   0, 255, 255};
inline const Color Transparent  = {  0,   0,   0,   0};

}

} // namespace uilo