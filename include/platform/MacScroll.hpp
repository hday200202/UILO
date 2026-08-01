#pragma once

#include <functional>

namespace uilo {

/* macOS only. */
bool installMacScrollMonitor(std::function<bool(float dyLines, float dxLines, bool momentum)> cb);

/* Call once per frame from your update loop, passing dt in seconds. */
void tickMacScrollMomentum(float dtSeconds);

/* Stop any active coast immediately. Call from elements that should not
   receive momentum (sliders, knobs, etc.) when a momentum tick reaches them. */
void cancelMacScrollMomentum();

/* Trackpad pinch (NSEventTypeMagnify). */
bool installMacZoomMonitor(std::function<bool(float magnification)> cb);

} // namespace uilo
