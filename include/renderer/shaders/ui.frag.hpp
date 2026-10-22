#pragma once

#include "ui.vert.hpp"

#include <algorithm>
#include <cmath>

inline float uiSmoothstep(const float a, const float b, const float x) {
    const float t = std::clamp((x - a) / (b - a), 0.f, 1.f);
    return t * t * (3.f - 2.f * t);
}

/*
    Coverage of a pixel against one rounded-rect clip. The antialiasing width is
    the screen-space gradient of the distance, |dd/dx| + |dd/dy|, which is what
    fwidth(d) measured on the GPU. It is computed exactly here instead, since the
    gradient of a rounded box is known: one along the straight edges and the
    interior, and the unit vector from the corner centre round a corner.
*/
inline float uiRoundedAlpha(const valo::Vec2f& p, const valo::Vec4f& rect, const valo::Vec4f& params) {
    if (params.y < 0.5f) return 1.f;

    const float r  = params.x;
    const float qx = std::abs(p.x - rect.x) - rect.z + r;
    const float qy = std::abs(p.y - rect.y) - rect.w + r;
    const float ox = std::max(qx, 0.f);
    const float oy = std::max(qy, 0.f);
    const float d  = std::sqrt(ox * ox + oy * oy) + std::min(std::max(qx, qy), 0.f) - r;

    float aa = 1.f;
    if (qx > 0.f && qy > 0.f) {
        const float len = std::sqrt(qx * qx + qy * qy);
        aa = (qx + qy) / len;
    }
    aa = aa + 1e-5f;

    return 1.f - uiSmoothstep(-aa, aa, d);
}

inline void uiFS(const UiVaryings& in, const UiUniforms& u) {
    using namespace valo;

    Vec4f c = in.color;

    if (u.mode == 1u) {
        c = texture(u.tex, in.uv, Sampler(Wrap::Clamp, Wrap::Clamp, Filter::Linear)) * in.color;

        // elliptical mask for round avatars; the width is fwidth(r2), from the
        // uv-per-pixel the renderer passes in
        if (u.imgFlags.x > 0.5f) {
            const float dx = in.uv.x - 0.5f;
            const float dy = in.uv.y - 0.5f;
            const float r2 = (dx * dx + dy * dy) * 4.f;
            const float aa = 8.f * (std::abs(dx) * u.imgFlags.y + std::abs(dy) * u.imgFlags.z) + 1e-5f;
            c.w = c.w * (1.f - uiSmoothstep(1.f - aa, 1.f + aa, r2));
        }
    } else if (u.mode == 2u) {
        const float a = texture(u.tex, in.uv, Sampler(Wrap::Clamp, Wrap::Clamp, Filter::Linear)).x;
        c.w = in.color.w * a;
    }

    c.w = c.w * uiRoundedAlpha(in.world, u.clipRect,  u.clipParams)
              * uiRoundedAlpha(in.world, u.clipRect2, u.clipParams2);

    if (c.w <= 0.f) discard;
    valo_FragColor = c;
}

VALO_FRAGMENT_SHADER(uiFS)
