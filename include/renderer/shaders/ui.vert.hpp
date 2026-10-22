#pragma once

#include <render/ShaderRegistry.hpp>

/*
    The one program every UILO draw goes through. Solid shapes, images and text
    differ only in how the fragment half colours a pixel, picked by `mode`, so a
    single program covers the whole renderer and a batch never has to switch
    pipelines between them.

    Geometry arrives already in window pixels (top-left origin), exactly as the
    renderer tessellates it, and the vertex half maps it to clip space against
    the target size. `world` carries the same pixel position through to the
    fragment half, which is what the rounded-clip masks are tested against.
*/

struct UiVertex {
    valo::Vec2f pos;        // window pixels
    valo::Vec4f color;      // 0..1, straight alpha
    valo::Vec2f uv;
};

struct UiUniforms {
    valo::Vec4f clipRect;       // inner rounded clip: centre xy, half size zw (px)
    valo::Vec4f clipParams;     // x = radius (px), y = enabled (> 0.5)
    valo::Vec4f clipRect2;      // outer rounded clip, same layout
    valo::Vec4f clipParams2;
    valo::Vec4f imgFlags;       // x = ellipse mask on, y = |du/dx|, z = |dv/dy|
    valo::Vec2f viewport;       // target size in pixels
    valo::TextureID tex;
    uint32_t    mode;           // 0 = solid, 1 = image, 2 = text (atlas coverage)
};

struct UiVaryings {
    valo::Vec4f color;
    valo::Vec2f uv;
    valo::Vec2f world;
};

inline void uiVS(const UiVertex& in, const UiUniforms& u, UiVaryings& out) {
    out.color = in.color;
    out.uv    = in.uv;
    out.world = in.pos;

    // pixels -> clip space; VALO's clip space is y-up, the window's y runs down
    const float x = in.pos.x / u.viewport.x * 2.f - 1.f;
    const float y = 1.f - in.pos.y / u.viewport.y * 2.f;
    valo::valo_Position = valo::Vec4f(x, y, 0.5f, 1.f);
}

VALO_VERTEX_SHADER(uiVS)
