// Internal header — included only by Renderer*.cpp translation units.
// Pulls in bgfx + stb_truetype + all caches.
#pragma once

#include "Renderer.hpp"

#include <bgfx/bgfx.h>
#include <bgfx/platform.h>

#include <string>
#include <unordered_map>
#include <vector>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <algorithm>

// stb_truetype declaration only — STB_TRUETYPE_IMPLEMENTATION lives in
// Renderer_Text.cpp.
#include "stb_truetype.h"

namespace uilo {

struct PosColorVertex {
    float    x, y;
    uint32_t abgr;
};

struct PosColorUvVertex {
    float    x, y;
    uint32_t abgr;
    float    u, v;
};

// ---- Cached glyph in a font atlas ----------------------------------------
struct Glyph {
    uint16_t x, y, w, h;   // pixel rect inside atlas
    float    xoff, yoff;   // offset from pen position to top-left of bitmap
    float    xadvance;     // horizontal advance
};

// A baked font at a specific pixel size.
struct FontFace {
    std::vector<uint8_t>             ttfData;   // owning copy of TTF bytes
    stbtt_fontinfo                   info{};
    float                            pixelHeight = 0.f;
    float                            scale       = 1.f;
    float                            ascent      = 0.f;
    float                            descent     = 0.f;
    float                            lineGap     = 0.f;
    int                              atlasW      = 0;
    int                              atlasH      = 0;
    int                              penX        = 0;
    int                              penY        = 0;
    int                              rowH        = 0;
    bgfx::TextureHandle              atlas       = BGFX_INVALID_HANDLE;
    std::unordered_map<uint32_t, Glyph> glyphs;
};

struct Renderer::Impl {
    // ---- bgfx shader programs ----
    bgfx::VertexLayout              solidLayout;
    bgfx::VertexLayout              texLayout;
    bgfx::ProgramHandle             solidProgram = BGFX_INVALID_HANDLE;
    bgfx::ProgramHandle             texProgram   = BGFX_INVALID_HANDLE;
    bgfx::ProgramHandle             textProgram  = BGFX_INVALID_HANDLE;
    bgfx::ProgramHandle             blurProgram  = BGFX_INVALID_HANDLE;
    bgfx::ProgramHandle             glassProgram = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle             s_texColor   = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle             u_imgFlags   = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle             u_blurParams = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle             u_glassParams= BGFX_INVALID_HANDLE;
    bgfx::UniformHandle             u_glassTint  = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle             u_glassRect  = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle             u_glassAnim  = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle             u_glassBase  = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle             u_glassMouse = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle             u_clipRect   = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle             u_clipParams = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle             u_clipRect2  = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle             u_clipParams2= BGFX_INVALID_HANDLE;
    bool                            layoutsInit  = false;

    // Wall-clock elapsed seconds since renderer init, fed into animated
    // materials (Holographic / Liquid / Shimmer / Aurora) via u_glassAnim.y.
    float                           elapsed      = 0.f;

    // ---- Offscreen scene + blur ladder (for Material::Glass) ----
    // Layout per frame (bgfx executes views in ascending viewId order):
    //   View 0: app draws the non-glass scene into sceneFB.
    //   View 1: horizontal blur, sceneFB -> blurFB_A (half-res).
    //   View 2: vertical   blur, blurFB_A -> blurFB_B (half-res, final blur).
    //   View 3: replay deferred glass draws into sceneFB, sampling blurFB_B.
    //   View 4: composite sceneFB -> backbuffer.
    // Glass elements thus sample a blur built from the SAME frame's scene
    // minus the glass elements themselves — no one-frame lag, no self-blur.
    bgfx::FrameBufferHandle         sceneFB       = BGFX_INVALID_HANDLE;
    bgfx::FrameBufferHandle         blurFB_A      = BGFX_INVALID_HANDLE;
    bgfx::FrameBufferHandle         blurFB_B      = BGFX_INVALID_HANDLE;
    bgfx::TextureHandle             sceneColorTex = BGFX_INVALID_HANDLE;
    bgfx::TextureHandle             blurColorA    = BGFX_INVALID_HANDLE;
    bgfx::TextureHandle             blurColorB    = BGFX_INVALID_HANDLE;
    uint32_t                        fbWidth       = 0;
    uint32_t                        fbHeight      = 0;
    static constexpr uint16_t       kSceneViewId        = 0;
    static constexpr uint16_t       kBlurHViewId        = 1;
    static constexpr uint16_t       kBlurVViewId        = 2;
    static constexpr uint16_t       kGlassBgViewId      = 3;
    static constexpr uint16_t       kGlassChildViewId   = 4;
    static constexpr uint16_t       kCompositeViewId    = 5;

    // ---- Deferred Material::Kind::* ("glass") draws ---------------------
    // drawGlass() captures here during the main render pass instead of
    // submitting immediately, so the blur ladder can run over a sceneFB
    // that excludes glass elements. endFrame() then replays the queue
    // (in submission order) into kGlassBgViewId.
    struct DeferredGlass {
        Rectf       dst;
        Material    mat;
        Color       baseColor;
        bool        hasScissor = false;
        uint16_t    sx = 0, sy = 0, sw = 0, sh = 0;
    };
    std::vector<DeferredGlass> deferredGlass;
    bool                       replayingGlass = false;

    // ---- Glass-pipeline bypass --------------------------------------------
    // The full glass pipeline (render scene to sceneFB → blur ladder →
    // composite back to backbuffer) costs an extra render-target switch,
    // a fullscreen blit, and a clear per frame even when no glass element
    // is on screen. We avoid all of that by predicting on each frame: if
    // the previous frame had no glass draws, we bind view 0 directly to
    // the backbuffer and skip the entire endFrame pipeline. Any glass
    // call during a bypassed frame falls back to a flat tint and flips
    // `hadGlassThisFrame` so the next frame uses the FB path again.
    bool hadGlassLastFrame = false;
    bool hadGlassThisFrame = false;
    bool bypassSceneFb     = false;

    // ---- Scissor stack ----
    struct ScissorEntry { uint16_t x, y, w, h; };
    static constexpr int            kMaxScissor = 16;
    ScissorEntry                    scissorStack[kMaxScissor]{};
    int                             scissorTop = 0;

    // ---- Rounded-rect clip stack (SDF in fragment shaders) ----
    struct RoundClipEntry { float cx, cy, halfW, halfH, radius; };
    static constexpr int            kMaxRoundClip = 16;
    RoundClipEntry                  roundClipStack[kMaxRoundClip]{};
    int                             roundClipTop = 0;

    // Last clip uniform values actually pushed to bgfx. Used to dedup
    // setUniform calls when consecutive draws share the same clip state.
    float lastClipRect[4]    = { 1e30f, 0.f, 0.f, 0.f };
    float lastClipParams[4]  = { 1e30f, 0.f, 0.f, 0.f };
    float lastClipRect2[4]   = { 1e30f, 0.f, 0.f, 0.f };
    float lastClipParams2[4] = { 1e30f, 0.f, 0.f, 0.f };
    bool  lastClipValid      = false;

    // ---- Solid-rect batch ------------------------------------------------
    // All `draw(Rect&)` calls sharing the same view + scissor + round-clip
    // + rotation snapshot get coalesced into a single transient vertex
    // buffer + submit. The batch must be flushed whenever any of those
    // pipeline inputs change, so flush points are wired into every
    // push/pop helper and every non-Rect submit path.
    std::vector<PosColorVertex> solidBatchVerts;
    std::vector<uint16_t>       solidBatchIdx;
    uint16_t                    solidBatchView = UINT16_MAX;

    // ---- Affine rotation (applied CPU-side to draw vertices) -------------
    // Convention: degrees, +x at 0, +y at 90 (matches a (cos t, sin t)
    // direction vector in screen-pixel coords).
    struct RotState {
        float pivotX  = 0.f;
        float pivotY  = 0.f;
        float angleDeg = 0.f;
        float cosA    = 1.f;
        float sinA    = 0.f;
        bool  enabled = false;
    };
    RotState rotation;

    // Rotate a screen-space point around the current pivot. Identity when
    // rotation is disabled, so callers can pipe every emitted vertex through
    // this without a branch at each call site.
    inline void rotPt(float& x, float& y) const {
        if (!rotation.enabled) return;
        const float dx = x - rotation.pivotX;
        const float dy = y - rotation.pivotY;
        x = rotation.pivotX + dx * rotation.cosA - dy * rotation.sinA;
        y = rotation.pivotY + dx * rotation.sinA + dy * rotation.cosA;
    }

    // ---- Texture cache ----
    // path -> Texture
    std::unordered_map<std::string, Texture> textureCache;

    // ---- Font cache ----
    // path -> font index; faces stored sparsely per requested pixel size
    struct FontRecord {
        std::vector<uint8_t> ttfData;
        // map keyed by integer pixel height
        std::unordered_map<int, FontFace> sizes;
    };
    std::vector<FontRecord>                 fonts;
    std::unordered_map<std::string, uint32_t> fontByPath;

    // ---- Cursor cache (kept alive for lifetime of Renderer) ----
    std::unordered_map<int, void*>          cursors;  // CursorType -> SDL_Cursor*

    // ---- Shader & layout setup ----
    bool initShaders();
    void ensureLayouts();
    void shutdownResources();

    // ---- Offscreen FB management for glass effect ----
    // Recreates sceneFB / blurFB_A / blurFB_B if (width,height) changed.
    void ensureSceneFramebuffers(uint32_t width, uint32_t height);
    void destroySceneFramebuffers();
    // Submits views kBlurHViewId and kBlurVViewId; assumes sceneFB has been
    // rendered into during this or the previous frame.
    void runBlurPasses(uint32_t width, uint32_t height);
    // Submits view kCompositeViewId: full-screen blit of sceneFB to backbuffer.
    void compositeSceneToBackbuffer(uint32_t width, uint32_t height,
                                    const bgfx::VertexLayout& layout,
                                    bgfx::ProgramHandle program);

    // ---- Helpers ----
    FontFace* getFace(uint32_t fontId, float pixelHeight);
    const Glyph* getGlyph(FontFace& face, uint32_t codepoint);

    // Flushes any queued solid-rect batch as a single transient-buffer
    // submit. Must be called before any pipeline-state change (view,
    // scissor, round-clip, FB, rotation, non-Rect shader/program).
    void flushSolidBatch();
};

// ---- Vertex packing helpers ----
inline uint32_t packColor(Color c) {
    return (uint32_t(c.a) << 24) | (uint32_t(c.b) << 16) |
           (uint32_t(c.g) <<  8) |  uint32_t(c.r);
}

// Decode one UTF-8 codepoint; returns advance (number of bytes consumed)
// and writes codepoint to *out. Returns 0 on end-of-string.
inline int utf8Decode(const char* s, size_t remaining, uint32_t* out) {
    if (remaining == 0) { *out = 0; return 0; }
    uint8_t c = (uint8_t)s[0];
    if (c < 0x80u) { *out = c; return 1; }
    if (c < 0xC0u) { *out = '?'; return 1; }  // stray continuation
    if (c < 0xE0u && remaining >= 2) {
        *out = ((uint32_t)(c & 0x1F) << 6) | (uint8_t)(s[1] & 0x3F);
        return 2;
    }
    if (c < 0xF0u && remaining >= 3) {
        *out = ((uint32_t)(c & 0x0F) << 12) |
               ((uint32_t)(s[1] & 0x3F) <<  6) |
               (uint32_t)(s[2] & 0x3F);
        return 3;
    }
    if (remaining >= 4) {
        *out = ((uint32_t)(c & 0x07) << 18) |
               ((uint32_t)(s[1] & 0x3F) << 12) |
               ((uint32_t)(s[2] & 0x3F) <<  6) |
               (uint32_t)(s[3] & 0x3F);
        return 4;
    }
    *out = '?';
    return 1;
}

} // namespace uilo
