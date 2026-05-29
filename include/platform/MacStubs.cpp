// Cross-platform stubs for the Mac-only trackpad / live-resize shims.
// On Apple builds CMake compiles MacScroll.mm / MacWindow.mm (which
// include their own non-Apple stubs guarded by `#if !__APPLE__`). On
// every other platform CMake compiles this file instead so the symbols
// referenced by UILO.cpp still resolve.
#if !defined(__APPLE__)

#include "MacScroll.hpp"
#include "MacWindow.hpp"

namespace uilo {

bool configureMacWindowForLiveResize(void*) { return false; }

bool installMacScrollMonitor(std::function<bool(float, float, bool)>) { return false; }
bool installMacZoomMonitor  (std::function<bool(float)>)              { return false; }
void tickMacScrollMomentum  (float)                                    {}

} // namespace uilo

#endif
