/*
    MacStubs.cpp:
    - Desc: Cross-platform stubs for the macOS-only trackpad and live-resize
            shims. On Apple builds CMake compiles MacScroll.mm and MacWindow.mm
            instead, which carry their own non-Apple guards; everywhere else this
            file is compiled so the symbols UILO.cpp references still resolve and
            each one degrades to "not available" rather than failing to link.
*/
#if !defined(__APPLE__)

#include "MacScroll.hpp"
#include "MacWindow.hpp"

namespace uilo {

/*
    configureMacWindowForLiveResize(void*):
    - Params:   void*
    - Returns:  bool
    - Desc:     No CoreGraphics mode list here either, so OS falls back to the
                SDL-derived size.
*/
bool configureMacWindowForLiveResize(void*) { return false; }

/*
    getNativeDisplayPixelSize(uint32_t&, uint32_t&):
    - Params:   uint32_t&, uint32_t&
    - Returns:  bool
    - Desc:     No equivalent of the CoreGraphics mode list here; OS falls
                back to the SDL-derived sizes, which is the right answer on
                platforms that do not render at one resolution and scan out
                at another.
*/
bool getNativeDisplayPixelSize(uint32_t&, uint32_t&) { return false; }
/*
    getVirtualDisplaySize(uint32_t&, uint32_t&):
    - Params:   uint32_t&, uint32_t&
    - Returns:  bool
    - Desc:     No AppKit event monitor to install; SDL's own wheel handling is
                used unchanged, which is adequate everywhere except macOS.
*/
bool getVirtualDisplaySize(uint32_t&, uint32_t&)     { return false; }

/*
    installMacScrollMonitor(std::function<bool(float, float, bool)>):
    - Params:   std::function<bool(float, float, bool)>
    - Returns:  bool
    - Desc:     No AppKit magnification gesture to monitor; pinch zoom arrives,
                if at all, through SDL.
*/
bool installMacScrollMonitor(std::function<bool(float, float, bool)>) { return false; }
/*
    installMacZoomMonitor(std::function<bool(float)>):
    - Params:   std::function<bool(float)>
    - Returns:  bool
    - Desc:     Nothing to tick: momentum is synthesised only on macOS, where
                SDL drops the phase events AppKit sends.
*/
bool installMacZoomMonitor  (std::function<bool(float)>)              { return false; }
/*
    tickMacScrollMomentum(float):
    - Params:   float
    - Returns:  none
    - Desc:     Nothing to cancel, for the same reason.
*/
void tickMacScrollMomentum  (float)                                    {}
/*
    cancelMacScrollMomentum():
    - Params:   none
    - Returns:  none
    - Desc:     Nothing to cancel, since no momentum is ever synthesised here.
*/
void cancelMacScrollMomentum()                                         {}

} // namespace uilo

#endif
