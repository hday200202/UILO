#pragma once

#include <cstdint>
#include <vector>
#include "EmbeddedFont.hpp"

namespace uilo {

// The icon set is NOT included here on purpose: EmbeddedIcons.hpp defines
// Resources::icons and so needs Resources to be a complete type. It is included
// from utils/Resources.hpp instead. This header is the font only.

// Use the embedded DejaVu Sans font
inline const std::vector<uint8_t>& EMBEDDED_FONT = EMBEDDED_DEJAVUSANS_FONT;

} // namespace uilo
