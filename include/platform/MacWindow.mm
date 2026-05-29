#include "MacWindow.hpp"

#ifdef __APPLE__

#import <AppKit/AppKit.h>
#import <QuartzCore/CAMetalLayer.h>

namespace uilo {

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
} // namespace uilo

#endif
