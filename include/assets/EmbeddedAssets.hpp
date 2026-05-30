#pragma once

#include <cstdint>
#include <vector>
#include "EmbeddedFont.hpp"
#include "EmbeddedIcons.hpp"

namespace uilo {

// Use the embedded DejaVu Sans font
inline const std::vector<uint8_t>& EMBEDDED_FONT = EMBEDDED_DEJAVUSANS_FONT;

} // namespace uilo
