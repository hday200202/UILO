#pragma once

#include <cstdint>

namespace uilo {

/* Native pixel resolution of the panel driving the main display -- the real
   hardware pixels, e.g. */
bool getNativeDisplayPixelSize(uint32_t& outWidth, uint32_t& outHeight);

/* Size of the main display in virtual points -- 1470x956 in the example above.
   This is the coordinate space window sizes and mouse positions live in. */
bool getVirtualDisplaySize(uint32_t& outWidth, uint32_t& outHeight);

/* macOS only. */
bool configureMacWindowForLiveResize(void* nsWindowPtr);

} // namespace uilo
