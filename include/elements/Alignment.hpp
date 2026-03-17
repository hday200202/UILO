#pragma once

#include <cstdint>

namespace uilo {

enum class Align : uint8_t {
	NONE        = 0,
	TOP         = 1 << 0,
	BOTTOM      = 1 << 1,
	LEFT        = 1 << 2,
	RIGHT       = 1 << 3,
	CENTER_X    = 1 << 4,
	CENTER_Y    = 1 << 5,
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

}