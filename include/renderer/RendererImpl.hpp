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
    bgfx::UniformHandle             s_texColor   = BGFX_INVALID_HANDLE;
    bool                            layoutsInit  = false;

    // ---- Scissor stack ----
    struct ScissorEntry { uint16_t x, y, w, h; };
    static constexpr int            kMaxScissor = 16;
    ScissorEntry                    scissorStack[kMaxScissor]{};
    int                             scissorTop = 0;

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

    // ---- Helpers ----
    FontFace* getFace(uint32_t fontId, float pixelHeight);
    const Glyph* getGlyph(FontFace& face, uint32_t codepoint);
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
