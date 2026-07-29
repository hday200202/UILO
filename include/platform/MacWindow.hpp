#pragma once

#include <cstdint>

namespace uilo {

// Native pixel resolution of the panel driving the main display -- the real
// hardware pixels, e.g. 2560x1664 on a 13" MacBook Air.
//
// This is deliberately NOT the backing-store size. macOS runs scaled HiDPI
// modes: at a 1470x956 point desktop it renders a 2940x1912 framebuffer and the
// display pipeline scans that out to the 2560x1664 panel. Every SDL and
// CoreGraphics "pixel" query reports the 2940x1912 framebuffer, so the panel's
// real resolution has to be recovered from the mode list instead.
//
// Returns false (leaving the outputs untouched) on non-macOS or if the mode
// list can't be read, so callers fall back to the SDL-derived figures.
bool getNativeDisplayPixelSize(uint32_t& outWidth, uint32_t& outHeight);

// Size of the main display in virtual points -- 1470x956 in the example above.
// This is the coordinate space window sizes and mouse positions live in.
bool getVirtualDisplaySize(uint32_t& outWidth, uint32_t& outHeight);

// macOS only. Tweaks the NSWindow's content view + CAMetalLayer so the
// window doesn't bilinear-stretch the last drawable while the user is
// live-resizing it.
//
// Pass the NSWindow* retrieved via
//   SDL_GetPointerProperty(SDL_GetWindowProperties(w),
//                          SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, NULL).
//
// Returns true if the layer was configured, false on non-macOS or when
// the layer isn't a CAMetalLayer yet.
bool configureMacWindowForLiveResize(void* nsWindowPtr);

} // namespace uilo
