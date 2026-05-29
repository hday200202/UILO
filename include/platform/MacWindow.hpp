#pragma once

namespace uilo {

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
