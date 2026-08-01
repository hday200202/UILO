#include "Color.hpp"
#include <cctype>

namespace uilo {

/*
    hexByte(std::string_view s, size_t pos):
    - Params:   std::string_view s, size_t pos
    - Returns:  static uint8_t
    - Desc:     Parses two hex digits into a byte, returning 0 for anything that
                is not a hex pair so a malformed string yields black rather than
                garbage.
*/
static uint8_t hexByte(std::string_view s, size_t pos) {
    auto nibble = [](char c) -> uint8_t {
        if (c >= '0' && c <= '9') return uint8_t(c - '0');
        if (c >= 'a' && c <= 'f') return uint8_t(10 + c - 'a');
        if (c >= 'A' && c <= 'F') return uint8_t(10 + c - 'A');
        return 0;
    };
    return uint8_t((nibble(s[pos]) << 4) | nibble(s[pos + 1]));
}

/*
    fromHex(std::string_view hex):
    - Params:   std::string_view hex
    - Returns:  Color
    - Desc:     Strip leading '#'
*/
Color Color::fromHex(std::string_view hex) {
    if (!hex.empty() && hex[0] == '#') hex.remove_prefix(1);

    Color out;
    if (hex.size() >= 6) {
        out.r = hexByte(hex, 0);
        out.g = hexByte(hex, 2);
        out.b = hexByte(hex, 4);
        out.a = (hex.size() >= 8) ? hexByte(hex, 6) : 255;
    }
    return out;
}

} // namespace uilo
