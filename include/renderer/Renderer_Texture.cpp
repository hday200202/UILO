#include "RendererImpl.hpp"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO   // we'll feed a buffer; actually keep stdio for stbi_load
#undef  STBI_NO_STDIO
#include "stb_image.h"

namespace uilo {

namespace {
inline void applyRoundClipInner(Renderer::Impl& impl) {
    float rect[4]    = {0.f, 0.f, 0.f, 0.f};
    float params[4]  = {0.f, 0.f, 0.f, 0.f};
    float rect2[4]   = {0.f, 0.f, 0.f, 0.f};
    float params2[4] = {0.f, 0.f, 0.f, 0.f};
    int picked = 0;
    for (int i = impl.roundClipTop - 1; i >= 0 && picked < 2; --i) {
        const auto& c = impl.roundClipStack[i];
        if (c.radius <= 0.f) continue;
        if (picked == 0) {
            rect[0] = c.cx; rect[1] = c.cy;
            rect[2] = c.halfW; rect[3] = c.halfH;
            params[0] = c.radius; params[1] = 1.f;
        } else {
            rect2[0] = c.cx; rect2[1] = c.cy;
            rect2[2] = c.halfW; rect2[3] = c.halfH;
            params2[0] = c.radius; params2[1] = 1.f;
        }
        ++picked;
    }
    const bool same = impl.lastClipValid
        && rect[0]    == impl.lastClipRect[0]    && rect[1]    == impl.lastClipRect[1]
        && rect[2]    == impl.lastClipRect[2]    && rect[3]    == impl.lastClipRect[3]
        && params[0]  == impl.lastClipParams[0]  && params[1]  == impl.lastClipParams[1]
        && rect2[0]   == impl.lastClipRect2[0]   && rect2[1]   == impl.lastClipRect2[1]
        && rect2[2]   == impl.lastClipRect2[2]   && rect2[3]   == impl.lastClipRect2[3]
        && params2[0] == impl.lastClipParams2[0] && params2[1] == impl.lastClipParams2[1];
    if (same) return;
    if (bgfx::isValid(impl.u_clipRect))    bgfx::setUniform(impl.u_clipRect,    rect);
    if (bgfx::isValid(impl.u_clipParams))  bgfx::setUniform(impl.u_clipParams,  params);
    if (bgfx::isValid(impl.u_clipRect2))   bgfx::setUniform(impl.u_clipRect2,   rect2);
    if (bgfx::isValid(impl.u_clipParams2)) bgfx::setUniform(impl.u_clipParams2, params2);
    impl.lastClipRect[0]    = rect[0];    impl.lastClipRect[1]    = rect[1];
    impl.lastClipRect[2]    = rect[2];    impl.lastClipRect[3]    = rect[3];
    impl.lastClipParams[0]  = params[0];  impl.lastClipParams[1]  = params[1];
    impl.lastClipParams[2]  = params[2];  impl.lastClipParams[3]  = params[3];
    impl.lastClipRect2[0]   = rect2[0];   impl.lastClipRect2[1]   = rect2[1];
    impl.lastClipRect2[2]   = rect2[2];   impl.lastClipRect2[3]   = rect2[3];
    impl.lastClipParams2[0] = params2[0]; impl.lastClipParams2[1] = params2[1];
    impl.lastClipParams2[2] = params2[2]; impl.lastClipParams2[3] = params2[3];
    impl.lastClipValid = true;
}
inline void applyScissor(Renderer::Impl& impl) {
    if (impl.scissorTop > 0) {
        auto& sc = impl.scissorStack[impl.scissorTop - 1];
        bgfx::setScissor(sc.x, sc.y, sc.w, sc.h);
    }
    applyRoundClipInner(impl);
}
inline void applyRoundClip(Renderer::Impl& impl) {
    applyRoundClipInner(impl);
}
inline bool scissorEmpty(const Renderer::Impl& impl) {
    if (impl.scissorTop == 0) return false;
    const auto& sc = impl.scissorStack[impl.scissorTop - 1];
    return sc.w == 0 || sc.h == 0;
}
} // anon

Texture Renderer::loadTexture(const std::string& path) {
    auto& impl = *m_impl;
    auto it = impl.textureCache.find(path);
    if (it != impl.textureCache.end()) return it->second;

    int w = 0, h = 0, comp = 0;
    stbi_uc* pixels = stbi_load(path.c_str(), &w, &h, &comp, 4);
    if (!pixels) {
        std::fprintf(stderr, "[UILO] loadTexture: failed to load '%s': %s\n",
                     path.c_str(), stbi_failure_reason());
        Texture invalid;
        impl.textureCache.emplace(path, invalid);
        return invalid;
    }

    // bgfx::TextureFormat::RGBA8 expects RGBA bytes (matches stb_image's req_comp=4)
    const bgfx::Memory* mem = bgfx::copy(pixels, (uint32_t)(w * h * 4));
    stbi_image_free(pixels);

    bgfx::TextureHandle th = bgfx::createTexture2D(
        (uint16_t)w, (uint16_t)h, false, 1,
        bgfx::TextureFormat::RGBA8,
        BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP |
        BGFX_SAMPLER_MIN_ANISOTROPIC | BGFX_SAMPLER_MAG_ANISOTROPIC,
        mem);

    Texture tex;
    tex.handle = th.idx;
    tex.width  = (uint16_t)w;
    tex.height = (uint16_t)h;
    impl.textureCache.emplace(path, tex);
    return tex;
}

void Renderer::destroyTexture(Texture& tex) {
    if (!tex.valid()) return;
    bgfx::TextureHandle h{ tex.handle };
    bgfx::destroy(h);
    tex.handle = UINT16_MAX;
    // (We don't bother removing from cache; it'll be cleared on shutdown.)
}

void Renderer::drawImage(const Rectf& dst, const Texture& tex,
                          Color tint, Rectf uv,
                          bool flipH, bool flipV,
                          bool clipEllipse) {
    if (!tex.valid()) return;
    auto& impl = *m_impl;
    impl.flushSolidBatch();
    if (!bgfx::isValid(impl.texProgram) || scissorEmpty(impl)) return;

    float x = dst.position.x, y = dst.position.y;
    float w = dst.size.x,     h = dst.size.y;

    float u0 = uv.position.x, v0 = uv.position.y;
    float u1 = uv.position.x + uv.size.x;
    float v1 = uv.position.y + uv.size.y;
    if (flipH) std::swap(u0, u1);
    if (flipV) std::swap(v0, v1);

    uint32_t col = packColor(tint);

    float x0 = x,     y0 = y;
    float x1 = x + w, y1 = y;
    float x2 = x + w, y2 = y + h;
    float x3 = x,     y3 = y + h;
    impl.rotPt(x0, y0); impl.rotPt(x1, y1);
    impl.rotPt(x2, y2); impl.rotPt(x3, y3);

    PosColorUvVertex verts[4] = {
        {x0, y0, col, u0, v0},
        {x1, y1, col, u1, v0},
        {x2, y2, col, u1, v1},
        {x3, y3, col, u0, v1},
    };
    uint16_t idx[6] = {0,1,2, 0,2,3};

    bgfx::TransientVertexBuffer tvb;
    bgfx::TransientIndexBuffer  tib;
    if (!bgfx::allocTransientBuffers(&tvb, impl.texLayout, 4, &tib, 6)) return;
    std::memcpy(tvb.data, verts, sizeof(verts));
    std::memcpy(tib.data, idx,   sizeof(idx));

    bgfx::TextureHandle th{ tex.handle };
    bgfx::setTexture(0, impl.s_texColor, th);
    {
        const float flags[4] = { clipEllipse ? 1.f : 0.f, 0.f, 0.f, 0.f };
        bgfx::setUniform(impl.u_imgFlags, flags);
    }
    bgfx::setVertexBuffer(0, &tvb);
    bgfx::setIndexBuffer(&tib);
    bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A |
                   BGFX_STATE_BLEND_ALPHA);
    applyScissor(impl);
    bgfx::submit(currentViewId(), impl.texProgram);
}

// ---------------------------------------------------------------------------
//  drawGlass — sample the blurred backdrop (built in the previous frame)
//  and overlay tint + edge highlight via the fs_glass shader.
// ---------------------------------------------------------------------------
void Renderer::drawGlass(const Rectf& dst, const Material& mat,
                         Color baseColor) {
    if (mat.kind == Material::Kind::None) return;
    auto& impl = *m_impl;
    impl.flushSolidBatch();
    if (!bgfx::isValid(impl.glassProgram)) return;
    if (scissorEmpty(impl))                return;
    if (dst.size.x <= 0.f || dst.size.y <= 0.f) return;

    // Mark glass-presence so the next frame switches back to the FB
    // pipeline. When this frame is in bypass mode (the predictor said
    // no glass), there's no sceneFB to defer into and no blur ladder
    // to feed — draw a flat tint as a one-frame visual fallback while
    // the FB path comes back online next frame.
    impl.hadGlassThisFrame = true;
    if (impl.bypassSceneFb) {
        Rect r;
        r.position  = dst.position;
        r.size      = dst.size;
        r.fillColor = baseColor;
        draw(r);
        return;
    }

    // Defer until endFrame so the blur ladder runs over a sceneFB that
    // doesn't contain any glass elements. During the replay pass we set
    // replayingGlass=true and re-enter here to actually submit.
    if (!impl.replayingGlass) {
        Impl::DeferredGlass d;
        d.dst        = dst;
        d.mat        = mat;
        d.baseColor  = baseColor;
        if (impl.scissorTop > 0) {
            const auto& s = impl.scissorStack[impl.scissorTop - 1];
            d.hasScissor = true;
            d.sx = s.x; d.sy = s.y; d.sw = s.w; d.sh = s.h;
        }
        impl.deferredGlass.push_back(d);
        return;
    }

    if (!bgfx::isValid(impl.blurFB_B))     return;

    // Backdrop UV: map the element's screen-space rect into the half-res
    // blur target's UV space. blurColorB shares aspect ratio with the
    // window (just at half resolution), so plain windowSize works.
    const float W = (float)m_lastWidth;
    const float H = (float)m_lastHeight;
    if (W <= 0.f || H <= 0.f) return;

    float x = dst.position.x, y = dst.position.y;
    float w = dst.size.x,     h = dst.size.y;

    // Some renderers (GL/ES) put the FB origin at the bottom-left; we need
    // to flip V when sampling our offscreen blur target.
    const bool flipV = bgfx::getCaps()->originBottomLeft;
    float u0 = x / W,         u1 = (x + w) / W;
    float v0 = y / H,         v1 = (y + h) / H;
    if (flipV) { v0 = 1.f - v0; v1 = 1.f - v1; }

    // v_color0 is packed with local 0..1 coords (in .rg via the .r/.g
    // bytes). The fragment shader reads it as a float vec4 normalised to
    // 0..1, so 0->0.0 and 255->1.0 — i.e. plain colour encoding works.
    auto pack01 = [](float a, float b) -> uint32_t {
        uint8_t ra = (uint8_t)std::clamp(a * 255.f, 0.f, 255.f);
        uint8_t rb = (uint8_t)std::clamp(b * 255.f, 0.f, 255.f);
        // ABGR layout (see packColor): r=byte0, g=byte1, b=byte2, a=byte3.
        return (uint32_t)0xff000000u | (uint32_t)rb << 8 | (uint32_t)ra;
    };

    PosColorUvVertex verts[4] = {
        {x,     y,     pack01(0.f, 0.f), u0, v0},
        {x + w, y,     pack01(1.f, 0.f), u1, v0},
        {x + w, y + h, pack01(1.f, 1.f), u1, v1},
        {x,     y + h, pack01(0.f, 1.f), u0, v1},
    };
    uint16_t idx[6] = {0,1,2, 0,2,3};

    bgfx::TransientVertexBuffer tvb;
    bgfx::TransientIndexBuffer  tib;
    if (!bgfx::allocTransientBuffers(&tvb, impl.texLayout, 4, &tib, 6)) return;
    std::memcpy(tvb.data, verts, sizeof(verts));
    std::memcpy(tib.data, idx,   sizeof(idx));

    // ---- Uniforms ----
    const float params[4] = {
        mat.opacity,                  // x = body opacity
        mat.saturation,
        mat.brightness,
        mat.edgeHighlight,
    };
    const float tintRgba[4] = {
        mat.tint.r / 255.f,
        mat.tint.g / 255.f,
        mat.tint.b / 255.f,
        mat.tint.a / 255.f,
    };
    // Refraction is authored in pixels; convert to UV units of the
    // (full-window) blur target so the shader can offset v_texcoord0
    // directly. Use the smaller axis so the bend reads similarly on
    // wide and tall panels.
    const float refractUv = mat.refraction / std::max(1.f, std::min(W, H));
    const float rect[4] = {
        mat.cornerRadius,     // x = corner radius (px)
        w * 0.5f,             // y = half width  (px)
        h * 0.5f,             // z = half height (px)
        refractUv,            // w = refraction strength (UV units)
    };
    bgfx::setUniform(impl.u_glassParams, params);
    bgfx::setUniform(impl.u_glassTint,   tintRgba);
    bgfx::setUniform(impl.u_glassRect,   rect);
    // Per-kind animation block: x=kind, y=time, z=speed, w=strength
    // For Material::Blur the w slot is repurposed to carry the per-element
    // blur radius (in pixels) instead — that kind isn't animated, so the
    // overload is safe.
    const bool  isBlurKind = (mat.kind == Material::Kind::Blur);
    const float anim[4] = {
        (float)(int)mat.kind,
        impl.elapsed,
        mat.animSpeed,
        isBlurKind ? mat.blurRadius : mat.animStrength,
    };
    bgfx::setUniform(impl.u_glassAnim, anim);

    // Element's own colour (for Tinted / Ripple / Hover). Transparent
    // signals "no base colour" — the shader will skip the overlay branch.
    const float base[4] = {
        baseColor.r / 255.f,
        baseColor.g / 255.f,
        baseColor.b / 255.f,
        baseColor.a / 255.f,
    };
    bgfx::setUniform(impl.u_glassBase, base);

    // Cursor in element-local 0..1 coords + activity signals. We compute
    // both axes regardless of whether the cursor sits inside the rect so
    // the shader can still render a soft halo on the rim when the user
    // approaches it.
    const Vec2f mpx       = m_mousePos;
    const float localU    = (mpx.x - x) / std::max(1.f, w);
    const float localV    = (mpx.y - y) / std::max(1.f, h);
    const float inside    = (localU >= 0.f && localU <= 1.f &&
                             localV >= 0.f && localV <= 1.f) ? 1.f : 0.f;
    const float sinceMove = std::max(0.f, impl.elapsed - m_mouseLastMoveT);
    const float mouseUni[4] = { localU, localV, inside, sinceMove };
    bgfx::setUniform(impl.u_glassMouse, mouseUni);

    bgfx::setTexture(0, impl.s_texColor, impl.blurColorB);

    bgfx::setVertexBuffer(0, &tvb);
    bgfx::setIndexBuffer(&tib);
    bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A |
                   BGFX_STATE_BLEND_ALPHA);
    applyScissor(impl);
    bgfx::submit(currentViewId(), impl.glassProgram);
}

} // namespace uilo
