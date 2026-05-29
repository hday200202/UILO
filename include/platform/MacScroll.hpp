#pragma once

#include <functional>

namespace uilo {

// macOS only. Installs an NSEvent local monitor for scroll-wheel events that
// reads precise pixel deltas and momentum-phase events directly from AppKit,
// bypassing SDL3's lossy Cocoa wheel handling (SDL3 drops momentum events
// and uses line-quantized deltaY instead of scrollingDeltaY).
//
// For trackpad events (hasPreciseScrollingDeltas), the monitor:
//   - converts pixel deltas into "lines" (divided by 30) so the value
//     matches the existing SDL wheel-delta convention
//   - invokes the callback with (dyLines, dxLines)
//   - consumes the event so SDL never sees it (no double dispatch)
//
// For non-precise mouse-wheel events the monitor passes the event through
// to SDL untouched.
//
// Returns true on success (callback installed) or false on non-macOS builds.
// Calling more than once replaces the previous callback.
// Installs an NSEvent local monitor for trackpad scroll-wheel events.
//
// The supplied callback is invoked for every precise scroll event AND for
// each synthesized momentum tick. It must return true if the host consumed
// the event (the monitor will then suppress it from SDL), or false to let
// SDL deliver it normally (use this for elements like sliders/knobs that
// should get raw wheel events without momentum).
//
// Non-precise events (real mouse wheels) are always passed through to SDL.
//
// No-op on non-macOS platforms (returns false).
bool installMacScrollMonitor(std::function<bool(float dyLines, float dxLines, bool momentum)> cb);

// Call once per frame from your update loop, passing dt in seconds. Drives
// the synthesized momentum tail (macOS does not generate momentum-phase
// events for non-NSScrollView windows, so we synthesize them ourselves
// from the velocity sampled at gesture end). No-op on non-macOS or when
// not coasting.
void tickMacScrollMomentum(float dtSeconds);

// Stop any active coast immediately. Call from elements that should not
// receive momentum (sliders, knobs, etc.) when a momentum tick reaches them.
void cancelMacScrollMomentum();

// Trackpad pinch (NSEventTypeMagnify). magnification is the per-event
// delta (NSEvent.magnification), already normalized so that the running
// sum across a gesture roughly equals (finalScale - 1). Callback returns
// true if consumed (always consumed from the OS regardless).
bool installMacZoomMonitor(std::function<bool(float magnification)> cb);

} // namespace uilo
