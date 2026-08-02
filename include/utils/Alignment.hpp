#pragma once

#include <cstdint>

namespace uilo {

/*
    Align:
    - Desc:     Where an element sits inside the slot its parent gives it. The
                horizontal and vertical flags are independent bits, so they
                combine with `|` -- Align::Left | Align::CenterY places an
                element against the left edge, centred vertically. A container
                also reads the flags to bucket its children into start, centre
                and end groups before laying them out.
*/
enum class Align : uint8_t {
    NONE        = 0,
    Top         = 1 << 0,
    Bottom      = 1 << 1,
    Left        = 1 << 2,
    Right       = 1 << 3,
    CenterX     = 1 << 4,
    CenterY     = 1 << 5,
};

/*
    operator|(Align lhs, Align rhs):
    - Params:   Align lhs, Align rhs
    - Returns:  Align
    - Desc:     Combines two alignment flags, which is how a horizontal and a
                vertical placement are given together.
*/
inline Align operator|(Align lhs, Align rhs) {
    return static_cast<Align>(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}

/*
    operator&(Align lhs, Align rhs):
    - Params:   Align lhs, Align rhs
    - Returns:  Align
    - Desc:     Masks alignment flags.
*/
inline Align operator&(Align lhs, Align rhs) {
    return static_cast<Align>(static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs));
}

/*
    hasAlign(Align value, Align flag):
    - Params:   Align value, Align flag
    - Returns:  bool
    - Desc:     Whether a flag is set, which is how layout tests one axis of a
                combined alignment without disturbing the other.
*/
inline bool hasAlign(Align value, Align flag) {
    return (static_cast<uint8_t>(value) & static_cast<uint8_t>(flag)) != 0;
}

} // namespace uilo