#pragma once

#include <cstdint>
#include <vector>
#include "EmbeddedFont.hpp"
#include "EmbeddedMonoFont.hpp"

namespace uilo {

// The icon set is NOT included here on purpose: EmbeddedIcons.hpp defines
// Resources::icons and so needs Resources to be a complete type. It is included
// from utils/Resources.hpp instead. This header is the font only.

// The two faces every UILO binary carries, so text renders with no files
// beside the executable. DejaVu Sans is the proportional one; Droid Sans Mono
// is the monospaced one, which anything laid out as a grid -- Terminal, a
// Textbox acting as a code editor -- needs to line up.
inline const std::vector<uint8_t>& EMBEDDED_FONT      = EMBEDDED_DEJAVUSANS_FONT;
inline const std::vector<uint8_t>& EMBEDDED_MONO_FONT = EMBEDDED_DROIDSANSMONO_FONT;

} // namespace uilo
