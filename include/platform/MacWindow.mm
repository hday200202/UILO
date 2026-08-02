#include "MacWindow.hpp"

#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif

/* __APPLE__ is true on iOS as well as macOS, and none of AppKit exists there.
   TARGET_OS_OSX is what actually distinguishes the two. */
#if defined(__APPLE__) && TARGET_OS_OSX

#import <AppKit/AppKit.h>
#import <CoreGraphics/CoreGraphics.h>
#import <QuartzCore/CAMetalLayer.h>

namespace uilo {

bool getNativeDisplayPixelSize(uint32_t& outWidth, uint32_t& outHeight) {
    const CGDirectDisplayID display = CGMainDisplayID();

    // kCGDisplayShowDuplicateLowResolutionModes is required or the 1:1 modes are
    // hidden and only the HiDPI ones come back.
    NSDictionary* options = @{ (id)kCGDisplayShowDuplicateLowResolutionModes : @YES };
    CFArrayRef modes = CGDisplayCopyAllDisplayModes(display, (CFDictionaryRef)options);
    if (!modes) return false;

    size_t bestW = 0;
    size_t bestH = 0;
    for (CFIndex i = 0; i < CFArrayGetCount(modes); ++i) {
        CGDisplayModeRef mode = (CGDisplayModeRef)CFArrayGetValueAtIndex(modes, i);
        const size_t pixelW = CGDisplayModeGetPixelWidth(mode);
        const size_t pixelH = CGDisplayModeGetPixelHeight(mode);

        // Keep only modes that drive the panel 1:1. A HiDPI mode reports more
        // pixels than points because it is rendered large and downscaled, so
        // including those would report 3420x2224 as the "panel".
        if (pixelW != CGDisplayModeGetWidth(mode)) continue;
        if (pixelH != CGDisplayModeGetHeight(mode)) continue;

        // Compare by area, not width: 2560x1600 and 2560x1664 are both offered
        // and only the taller one is the actual panel.
        if (pixelW * pixelH > bestW * bestH) { bestW = pixelW; bestH = pixelH; }
    }
    CFRelease(modes);

    if (bestW == 0 || bestH == 0) return false;
    outWidth  = static_cast<uint32_t>(bestW);
    outHeight = static_cast<uint32_t>(bestH);
    return true;
}

bool getVirtualDisplaySize(uint32_t& outWidth, uint32_t& outHeight) {
    CGDisplayModeRef mode = CGDisplayCopyDisplayMode(CGMainDisplayID());
    if (!mode) return false;
    const size_t w = CGDisplayModeGetWidth(mode);
    const size_t h = CGDisplayModeGetHeight(mode);
    CGDisplayModeRelease(mode);
    if (w == 0 || h == 0) return false;
    outWidth  = static_cast<uint32_t>(w);
    outHeight = static_cast<uint32_t>(h);
    return true;
}

bool configureMacWindowForLiveResize(void* nsWindowPtr) {
    if (!nsWindowPtr) return false;
    NSWindow* win = (__bridge NSWindow*)nsWindowPtr;
    NSView*   view = [win contentView];
    if (!view) return false;

    // Without these, AppKit reflows the layer's existing contents to the
    // new bounds during a live resize with bilinear scaling — the visible
    // "stretch" until the next frame is presented. Top-left anchor keeps
    // the old pixels aligned to the corner instead of rescaling.
    [view setLayerContentsRedrawPolicy:NSViewLayerContentsRedrawDuringViewResize];
    [view setLayerContentsPlacement:NSViewLayerContentsPlacementTopLeft];

    CALayer* layer = [view layer];
    if (![layer isKindOfClass:[CAMetalLayer class]]) return false;
    CAMetalLayer* ml = (CAMetalLayer*)layer;
    ml.contentsGravity = kCAGravityTopLeft;
    // Intentionally NOT setting presentsWithTransaction = YES: bgfx's
    // Metal backend uses [commandBuffer presentDrawable:], not the
    // wait-until-scheduled + present sequence that the transactional
    // path requires, so enabling it can hang on the commit.
    return true;
}

} // namespace uilo

#else

namespace uilo {
bool configureMacWindowForLiveResize(void*) { return false; }
bool getNativeDisplayPixelSize(uint32_t&, uint32_t&) { return false; }
bool getVirtualDisplaySize(uint32_t&, uint32_t&)     { return false; }
} // namespace uilo

#endif
