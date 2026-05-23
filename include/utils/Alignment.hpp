#pragma once

#include <cstdint>

namespace uilo {

enum class Align : uint8_t {
    NONE        = 0,
    Top         = 1 << 0,
    Bottom      = 1 << 1,
    Left        = 1 << 2,
    Right       = 1 << 3,
    CenterX     = 1 << 4,
    CenterY     = 1 << 5,
};

inline Align operator|(Align lhs, Align rhs) {
    return static_cast<Align>(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}

inline Align operator&(Align lhs, Align rhs) {
    return static_cast<Align>(static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs));
}

inline bool hasAlign(Align value, Align flag) {
    return (static_cast<uint8_t>(value) & static_cast<uint8_t>(flag)) != 0;
}

} // namespace uilo