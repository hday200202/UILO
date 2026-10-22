/* Internal header — included only by Renderer*.cpp translation units. Pulls in
   VALO, the UI shader and stb_truetype. */
#pragma once

#include "Renderer.hpp"

#include <VALO.hpp>
#include <render/SpirvRegistry.hpp>
#include "shaders/ui.frag.hpp"

#include <string>
#include <unordered_map>
#include <vector>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <algorithm>

/* stb_truetype declaration only — STB_TRUETYPE_IMPLEMENTATION lives in
   Renderer_Text.cpp. */
#include "stb_truetype.h"

namespace uilo {

/* Cached glyph in a font atlas */
struct Glyph {
    uint16_t x, y, w, h;   /* pixel rect inside atlas */
    float    xoff, yoff;   /* offset from pen position to top-left of bitmap */
    float    xadvance;   /* horizontal advance */
};

/*
    FontFace:
    - Desc:     A font baked at one pixel size. The atlas lives twice: as RGBA8
                pixels on the CPU, where glyphs are rasterised as they are first
                needed, and as a VALO texture the draws sample. Newly packed
                glyphs mark a dirty rectangle, and the renderer uploads that
                rectangle once per frame, before the frame's draws run.
*/
struct FontFace {
    std::vector<uint8_t>             ttfData;   /* owning copy of TTF bytes */
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
    valo::TextureID                  atlas       = valo::EMPTY_TEXTURE;
    std::vector<uint32_t>            pixels;   /* coverage replicated into every channel */
    int                              dirtyX0 = 0, dirtyY0 = 0, dirtyX1 = 0, dirtyY1 = 0;
    bool                             dirty   = false;
    std::unordered_map<uint32_t, Glyph> glyphs;
};

/*
    Renderer::Impl:
    - Desc:     The renderer's private state, kept out of the public header so
                VALO's headers never leak into anything that includes
                Renderer.hpp. Holds the window and the VALO instance, the one UI
                program, the per-frame vertex arena and the batch being built in
                it, the clip and rotation stacks, and the texture and font
                caches.
    - Every shape is tessellated into the arena on the CPU and joins the pending
      batch when its draw state -- target, mode, texture, clips and scissor --
      matches; otherwise the batch is submitted and a new one starts. The arena
      is uploaded once, just before VALO runs the frame, so every command reads
      the frame's final contents.
*/
struct Renderer::Impl {
    std::unique_ptr<valo::Window> window;
    valo::VALO                    valo;
    valo::Backend                 backend      = valo::Backend::Software;
    bool                          vsync        = true;
    bool                          valoReady    = false;

    valo::ProgramID               program      = valo::DEFAULT_PROGRAM;
    valo::VtxLayout               layout;
    valo::VertexBufferID          vb           = valo::EMPTY_BUFFER;
    valo::IndexBufferID           ib           = valo::EMPTY_BUFFER;

    /* The frame's geometry: window pixels, uploaded once at endFrame. */
    std::vector<UiVertex>         verts;
    std::vector<uint32_t>         idx;

    /* The clear colour the frame starts from. clear() sets it; the last call
       in a frame wins, and it is applied before any of the frame's draws. */
    Color                         clearColor   = Color{0, 0, 0, 255};

    /* ---- draw targets ---------------------------------------------------- */
    /* Where draws go: the base layer (the window), or a framebuffer's layer
       while one is pushed. */
    struct TargetEntry { valo::LayerID layer; Vec2u size; };
    static constexpr int            kMaxTargetStack = 16;
    TargetEntry                     targetStack[kMaxTargetStack]{};
    int                             targetTop = 0;
    std::vector<valo::LayerID>      fbLayers;   /* framebuffer layers, drawn before the base */
    std::vector<valo::LayerID>      freeLayers;

    valo::LayerID currentLayer() const {
        return targetTop > 0 ? targetStack[targetTop - 1].layer : valo::VALO::BASE_LAYER;
    }
    Vec2u currentTargetSize(Vec2u windowSize) const {
        return targetTop > 0 ? targetStack[targetTop - 1].size : windowSize;
    }
    void applyLayerOrder();

    /* ---- batching -------------------------------------------------------- */
    enum Mode : uint32_t { ModeSolid = 0, ModeImage = 1, ModeText = 2 };

    struct Batch {
        bool            open       = false;
        valo::LayerID   layer      = 0;
        uint32_t        firstIndex = 0;
        uint32_t        indexCount = 0;
        UiUniforms      uniforms{};
        valo::Scissor   scissor{};
    };
    Batch                           batch;
    Vec2u                           frameSize   = {1u, 1u};

    /* Starts or extends the batch for a draw in `mode` sampling `tex`, under
       the current scissor and rounded clips. Returns false when the draw is
       fully scissored away. */
    bool beginShape(uint32_t mode, valo::TextureID tex,
                    const float imgFlags[4] = nullptr);
    void flushBatch();
    uint32_t submittedCommands = 0;
    RendererStats stats;   /* the previous frame's counters */

    /* Appends geometry to the arena under the batch beginShape() opened. */
    uint32_t pushVertex(float x, float y, Color c, float u = 0.f, float v = 0.f) {
        verts.push_back(UiVertex{
            valo::Vec2f{x, y},
            valo::Vec4f{c.r / 255.f, c.g / 255.f, c.b / 255.f, c.a / 255.f},
            valo::Vec2f{u, v}
        });
        return static_cast<uint32_t>(verts.size() - 1);
    }
    void pushIndex(uint32_t i) {
        idx.push_back(i);
        ++batch.indexCount;
    }

    /* ---- scissor stack --------------------------------------------------- */
    struct ScissorEntry { uint16_t x, y, w, h; };
    static constexpr int            kMaxScissor = 64;
    ScissorEntry                    scissorStack[kMaxScissor]{};
    int                             scissorTop = 0;
    int                             scissorOverflowDepth = 0;

    bool scissorEmpty() const {
        if (scissorTop == 0) return false;
        const auto& sc = scissorStack[scissorTop - 1];
        return sc.w == 0 || sc.h == 0;
    }

    /* ---- rounded-rect clip stack (SDF in the fragment shader) ------------- */
    struct RoundClipEntry { float cx, cy, halfW, halfH, radius; };
    static constexpr int            kMaxRoundClip = 64;
    RoundClipEntry                  roundClipStack[kMaxRoundClip]{};
    int                             roundClipTop = 0;
    int                             roundClipOverflowDepth = 0;

    /* The two vec4 pairs the fragment shader tests for the CURRENT stack:
       the inner-most and the next rounded entry. Rebuilt only when the stack
       has changed since it was last read. */
    float    curClipRect[4]    = {0.f, 0.f, 0.f, 0.f};
    float    curClipParams[4]  = {0.f, 0.f, 0.f, 0.f};
    float    curClipRect2[4]   = {0.f, 0.f, 0.f, 0.f};
    float    curClipParams2[4] = {0.f, 0.f, 0.f, 0.f};
    uint32_t clipVersion    = 1;
    uint32_t curClipVersion = 0;
    void refreshClipCache();

    /* ---- rotation (applied CPU-side to draw vertices) --------------------- */
    /* Convention: degrees, +x at 0, +y at 90 (matches a (cos t, sin t). */
    struct RotState {
        float pivotX  = 0.f;
        float pivotY  = 0.f;
        float angleDeg = 0.f;
        float cosA    = 1.f;
        float sinA    = 0.f;
        bool  enabled = false;
    };
    RotState rotation;

    /* Rotate a screen-space point around the current pivot. */
    inline void rotPt(float& x, float& y) const {
        if (!rotation.enabled) return;
        const float dx = x - rotation.pivotX;
        const float dy = y - rotation.pivotY;
        x = rotation.pivotX + dx * rotation.cosA - dy * rotation.sinA;
        y = rotation.pivotY + dx * rotation.sinA + dy * rotation.cosA;
    }

    /* ---- texture cache --------------------------------------------------- */
    /* path -> Texture */
    std::unordered_map<std::string, Texture> textureCache;

    /* ---- font cache ------------------------------------------------------ */
    /* path -> font index; faces stored sparsely per requested pixel size */
    struct FontRecord {
        std::vector<uint8_t> ttfData;
        /* map keyed by integer pixel height */
        std::unordered_map<int, FontFace> sizes;
    };
    std::vector<FontRecord>                 fonts;
    std::unordered_map<std::string, uint32_t> fontByPath;

    FontFace* getFace(uint32_t fontId, float pixelHeight);
    const Glyph* getGlyph(FontFace& face, uint32_t codepoint);
    /* Uploads every atlas region rasterised since the last frame. */
    void uploadDirtyAtlases();

    /* ---- lifecycle helpers ---------------------------------------------- */
    bool createResources();
    void shutdownResources();
};

/* Decode one UTF-8 codepoint; returns advance (number of bytes consumed) and
   writes codepoint to *out. Returns 0 on end-of-string. */
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
