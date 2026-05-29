#include "MacScroll.hpp"

#ifdef __APPLE__

#import <AppKit/AppKit.h>
#include <cstdio>
#include <array>
#include <cmath>

namespace {

std::function<bool(float, float, bool)> g_cb;
id g_monitor = nil;

// Velocity sampling. Keep the last ~120ms of (dxPx, dyPx, t) and derive
// a release-velocity at gesture end.
struct Sample { float dxPx; float dyPx; double t; };
constexpr int kRing = 16;
std::array<Sample, kRing> g_ring{};
int    g_ringHead = 0;
int    g_ringCount = 0;

// Synthesized momentum state.
bool   g_coasting       = false;
float  g_velXPxPerSec   = 0.f;
float  g_velYPxPerSec   = 0.f;
constexpr float kFrictionPerSec   = 6.0f;   // vel *= exp(-k * dt)
constexpr float kMinSpeedPxPerSec = 30.f;   // stop coasting below this
constexpr float kFlickThreshold   = 250.f;  // px/sec required to coast

void resetSamples() { g_ringHead = 0; g_ringCount = 0; }

void pushSample(float dxPx, float dyPx, double t) {
    g_ring[g_ringHead] = { dxPx, dyPx, t };
    g_ringHead = (g_ringHead + 1) % kRing;
    if (g_ringCount < kRing) g_ringCount++;
}

// px/sec on each axis averaged over the most recent ~80ms of samples.
void computeReleaseVelocity(double nowT, float& outVx, float& outVy) {
    outVx = 0.f; outVy = 0.f;
    if (g_ringCount < 2) return;
    constexpr double kWindow = 0.08;
    float  sumDx  = 0.f, sumDy = 0.f;
    double oldestT = nowT;
    int    used    = 0;
    for (int i = 0; i < g_ringCount; ++i) {
        int idx = (g_ringHead - 1 - i + kRing) % kRing;
        const Sample& s = g_ring[idx];
        if (nowT - s.t > kWindow && used >= 2) break;
        sumDx   += s.dxPx;
        sumDy   += s.dyPx;
        oldestT  = s.t;
        used++;
    }
    const double span = nowT - oldestT;
    if (span < 1e-4) return;
    outVx = (float)((double)sumDx / span);
    outVy = (float)((double)sumDy / span);
}

} // namespace

namespace uilo {

bool installMacScrollMonitor(std::function<bool(float, float, bool)> cb) {
    g_cb = std::move(cb);

    if (g_monitor) {
        [NSEvent removeMonitor:g_monitor];
        g_monitor = nil;
    }

    id mon = [NSEvent
        addLocalMonitorForEventsMatchingMask:NSEventMaskScrollWheel
        handler:^NSEvent*(NSEvent* event) {
            if (!event.hasPreciseScrollingDeltas) return event; // mouse wheel: pass to SDL

            const float dyPx = (float)event.scrollingDeltaY;
            const float dxPx = (float)event.scrollingDeltaX;
            const double now = event.timestamp;
            const NSEventPhase phase = event.phase;

            if (phase & (NSEventPhaseBegan | NSEventPhaseMayBegin)) {
                g_coasting    = false;
                g_velXPxPerSec = 0.f;
                g_velYPxPerSec = 0.f;
                resetSamples();
            }

            // Always query the host \u2014 even on zero-delta events \u2014 so we
            // get an up-to-date eligibility flag at gesture-end (where dy
            // is 0). The host's dispatchScroll early-returns on delta==0.
            bool eligibleForMomentum = false;
            if (g_cb) eligibleForMomentum = g_cb(dyPx / 30.0f, dxPx / 30.0f, /*momentum=*/false);

            if (eligibleForMomentum && (phase & (NSEventPhaseBegan | NSEventPhaseChanged)))
                pushSample(dxPx, dyPx, now);

            if (phase & NSEventPhaseEnded) {
                if (eligibleForMomentum) {
                    float vx = 0.f, vy = 0.f;
                    computeReleaseVelocity(now, vx, vy);
                    const float speed = std::sqrt(vx * vx + vy * vy);
                    if (speed >= kFlickThreshold) {
                        g_velXPxPerSec = vx;
                        g_velYPxPerSec = vy;
                        g_coasting     = true;
                    }
                }
                resetSamples();
            }
            if (phase & NSEventPhaseCancelled) {
                g_coasting    = false;
                g_velXPxPerSec = 0.f;
                g_velYPxPerSec = 0.f;
                resetSamples();
            }
            return nil; // always consume: SDL's deltaY is 0 for precise events anyway
        }];

    g_monitor = [mon retain];
    return g_monitor != nil;
}

void tickMacScrollMomentum(float dt) {
    if (!g_coasting || !g_cb || dt <= 0.f) return;
    const float dxPx = g_velXPxPerSec * dt;
    const float dyPx = g_velYPxPerSec * dt;
    if (dxPx != 0.f || dyPx != 0.f) {
        const bool stillEligible = g_cb(dyPx / 30.0f, dxPx / 30.0f, /*momentum=*/true);
        if (!stillEligible) {
            g_coasting     = false;
            g_velXPxPerSec = 0.f;
            g_velYPxPerSec = 0.f;
            return;
        }
    }
    const float decay = std::exp(-kFrictionPerSec * dt);
    g_velXPxPerSec *= decay;
    g_velYPxPerSec *= decay;
    const float speed = std::sqrt(g_velXPxPerSec * g_velXPxPerSec
                                + g_velYPxPerSec * g_velYPxPerSec);
    if (speed < kMinSpeedPxPerSec) {
        g_coasting     = false;
        g_velXPxPerSec = 0.f;
        g_velYPxPerSec = 0.f;
    }
}

void cancelMacScrollMomentum() {
    g_coasting     = false;
    g_velXPxPerSec = 0.f;
    g_velYPxPerSec = 0.f;
}

namespace { id g_zoomMonitor = nil; std::function<bool(float)> g_zoomCb; }

bool installMacZoomMonitor(std::function<bool(float)> cb) {
    g_zoomCb = std::move(cb);
    if (g_zoomMonitor) { [NSEvent removeMonitor:g_zoomMonitor]; g_zoomMonitor = nil; }
    id mon = [NSEvent
        addLocalMonitorForEventsMatchingMask:NSEventMaskMagnify
        handler:^NSEvent*(NSEvent* event) {
            const float mag = (float)event.magnification;
            if (g_zoomCb && mag != 0.f) g_zoomCb(mag);
            return nil; // consume; the host now owns pinch
        }];
    g_zoomMonitor = [mon retain];
    return g_zoomMonitor != nil;
}

} // namespace uilo

#else // !__APPLE__

namespace uilo {
bool installMacScrollMonitor(std::function<bool(float, float, bool)>) { return false; }
void tickMacScrollMomentum(float) {}
void cancelMacScrollMomentum() {}
bool installMacZoomMonitor(std::function<bool(float)>) { return false; }
}

#endif
