#include "RendererImpl.hpp"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#include "../assets/EmbeddedAssets.hpp"
#include "../utils/Resources.hpp"

#include <cstdio>
#include <cstdlib>
#include <cmath>

namespace uilo {

/* applyScissor / scissorEmpty / clip-uniform helpers are shared inlines in
   RendererImpl.hpp. */

namespace {

/*
    readFile(const char* path, std::vector<uint8_t>& out):
    - Params:   const char* path, std::vector<uint8_t>& out
    - Returns:  bool
    - Desc:     Slurp a file fully into a vector.
*/
bool readFile(const char* path, std::vector<uint8_t>& out) {
    std::FILE* f = std::fopen(path, "rb");
    if (!f) return false;
    std::fseek(f, 0, SEEK_END);
    long sz = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    if (sz <= 0) { std::fclose(f); return false; }
    out.resize((size_t)sz);
    size_t got = std::fread(out.data(), 1, (size_t)sz, f);
    std::fclose(f);
    return got == (size_t)sz;
}

constexpr int kAtlasW = 1024;
constexpr int kAtlasH = 1024;
constexpr const char* kEmbeddedFontCacheKey     = "__UILO_EMBEDDED_DEFAULT_FONT__";
constexpr const char* kEmbeddedMonoFontCacheKey = "__UILO_EMBEDDED_MONO_FONT__";

/*
    initAtlasFace(...):
    - Params:   FontFace& face, stbtt_fontinfo info, std::vector<uint8_t> ttf,
                float pixelHeight
    - Returns:  none
    - Desc:     Sets up a font face at one pixel size: reads the vertical
                metrics, scales them, and allocates a blank atlas the glyphs are
                packed into on demand. The GPU texture is created by the caller,
                which owns the VALO instance.
*/
void initAtlasFace(FontFace& face, stbtt_fontinfo info,
                   std::vector<uint8_t> ttf, float pixelHeight) {
    face.ttfData     = std::move(ttf);
    face.info        = info;
    face.pixelHeight = pixelHeight;
    face.scale       = stbtt_ScaleForPixelHeight(&face.info, pixelHeight);
    int asc = 0, desc = 0, lineGap = 0;
    stbtt_GetFontVMetrics(&face.info, &asc, &desc, &lineGap);
    face.ascent  =  asc     * face.scale;
    face.descent = -desc    * face.scale;   /* stb reports negative descent */
    face.lineGap =  lineGap * face.scale;
    face.atlasW  = kAtlasW;
    face.atlasH  = kAtlasH;
    face.penX = 1;
    face.penY = 1;
    face.rowH = 0;

    /* Blank coverage, held as RGBA8 with the coverage in every channel: VALO
       has no single-channel format yet, and the shader reads .x either way. */
    face.pixels.assign((size_t)kAtlasW * (size_t)kAtlasH, 0u);
}
} // anon

/*
    getFace(uint32_t fontId, float pixelHeight):
    - Params:   uint32_t fontId, float pixelHeight
    - Returns:  FontFace*
    - Desc:     The cached face for a font at a pixel size, created on first
                use. Sizes are cached separately because a glyph atlas is
                rasterised at one size; asking for a new size builds a new face
                rather than scaling an existing one, which would blur it.
*/
FontFace* Renderer::Impl::getFace(uint32_t fontId, float pixelHeight) {
    if (fontId >= fonts.size()) return nullptr;
    int key = (int)(pixelHeight + 0.5f);
    if (key < 1) key = 1;
    auto& rec = fonts[fontId];
    auto it = rec.sizes.find(key);
    if (it != rec.sizes.end()) return &it->second;

    /* Parse font info from owning ttfData */
    stbtt_fontinfo info{};
    if (!stbtt_InitFont(&info, rec.ttfData.data(),
                       stbtt_GetFontOffsetForIndex(rec.ttfData.data(), 0))) {
        return nullptr;
    }
    FontFace face;
    initAtlasFace(face, info, rec.ttfData, (float)key);
    /* Never sampled through mips: glyphs are drawn texel-for-pixel. */
    face.atlas = valo.createTexture((uint32_t)face.atlasW, (uint32_t)face.atlasH,
                                    face.pixels.data(), false, false);
    auto [insIt, ok] = rec.sizes.emplace(key, std::move(face));
    return &insIt->second;
}

/*
    getGlyph(FontFace& face, uint32_t codepoint):
    - Params:   FontFace& face, uint32_t codepoint
    - Returns:  const Glyph*
    - Desc:     The cached glyph for a codepoint, rasterised and packed into the
                atlas on first use. Packing runs left to right in rows; when the
                atlas is full the glyph is recorded as zero-sized so it simply
                does not draw, rather than corrupting the sheet.
*/
const Glyph* Renderer::Impl::getGlyph(FontFace& face, uint32_t codepoint) {
    auto it = face.glyphs.find(codepoint);
    if (it != face.glyphs.end()) return &it->second;

    int x0, y0, x1, y1;
    stbtt_GetCodepointBitmapBox(&face.info, (int)codepoint,
                                 face.scale, face.scale,
                                 &x0, &y0, &x1, &y1);
    int gw = x1 - x0;
    int gh = y1 - y0;

    int adv = 0, lsb = 0;
    stbtt_GetCodepointHMetrics(&face.info, (int)codepoint, &adv, &lsb);

    Glyph g{};
    g.xoff     = (float)x0;
    g.yoff     = (float)y0;   /* negative (above baseline) */
    g.xadvance = adv * face.scale;

    if (gw <= 0 || gh <= 0) {
        g.x = g.y = 0;
        g.w = g.h = 0;
        return &face.glyphs.emplace(codepoint, g).first->second;
    }

    /* Reserve space in atlas */
    const int pad = 1;
    if (face.penX + gw + pad >= face.atlasW) {
        face.penX  = 1;
        face.penY += face.rowH + pad;
        face.rowH  = 0;
    }
    if (face.penY + gh + pad >= face.atlasH) {
        /* Atlas full — give up on this glyph (no visible draw) */
        g.w = g.h = 0;
        return &face.glyphs.emplace(codepoint, g).first->second;
    }

    /* Render glyph into temp buffer */
    std::vector<uint8_t> bmp((size_t)gw * (size_t)gh, 0);
    stbtt_MakeCodepointBitmap(&face.info, bmp.data(), gw, gh, gw,
                              face.scale, face.scale, (int)codepoint);

    /* Into the CPU atlas, coverage replicated into every channel. The region
       is uploaded with the rest of the frame's glyphs before the frame runs. */
    for (int row = 0; row < gh; ++row) {
        uint32_t* dst = face.pixels.data() + (size_t)(face.penY + row) * (size_t)face.atlasW
                      + (size_t)face.penX;
        const uint8_t* src = bmp.data() + (size_t)row * (size_t)gw;
        for (int col = 0; col < gw; ++col) dst[col] = 0x01010101u * src[col];
    }
    if (!face.dirty) {
        face.dirtyX0 = face.penX;      face.dirtyY0 = face.penY;
        face.dirtyX1 = face.penX + gw; face.dirtyY1 = face.penY + gh;
        face.dirty   = true;
    } else {
        face.dirtyX0 = std::min(face.dirtyX0, face.penX);
        face.dirtyY0 = std::min(face.dirtyY0, face.penY);
        face.dirtyX1 = std::max(face.dirtyX1, face.penX + gw);
        face.dirtyY1 = std::max(face.dirtyY1, face.penY + gh);
    }

    g.x = (uint16_t)face.penX;
    g.y = (uint16_t)face.penY;
    g.w = (uint16_t)gw;
    g.h = (uint16_t)gh;
    face.penX += gw + pad;
    if (gh > face.rowH) face.rowH = gh;

    return &face.glyphs.emplace(codepoint, g).first->second;
}

/*
    uploadDirtyAtlases():
    - Params:   none
    - Returns:  none
    - Desc:     Sends every atlas region rasterised since the last frame to its
                texture, one rectangle per face. Runs at endFrame, before VALO
                executes the frame, so a glyph first drawn this frame is already
                on the GPU when its quad is.
*/
void Renderer::Impl::uploadDirtyAtlases() {
    std::vector<uint32_t> region;
    for (auto& rec : fonts) {
        for (auto& kv : rec.sizes) {
            FontFace& face = kv.second;
            if (!face.dirty || face.atlas == valo::EMPTY_TEXTURE) continue;
            face.dirty = false;

            const int w = face.dirtyX1 - face.dirtyX0;
            const int h = face.dirtyY1 - face.dirtyY0;
            if (w <= 0 || h <= 0) continue;

            region.resize((size_t)w * (size_t)h);
            for (int row = 0; row < h; ++row) {
                const uint32_t* src = face.pixels.data()
                                    + (size_t)(face.dirtyY0 + row) * (size_t)face.atlasW
                                    + (size_t)face.dirtyX0;
                std::memcpy(region.data() + (size_t)row * (size_t)w, src, (size_t)w * sizeof(uint32_t));
            }
            valo.updateTexture(face.atlas, (uint32_t)face.dirtyX0, (uint32_t)face.dirtyY0,
                               (uint32_t)w, (uint32_t)h, region.data());
        }
    }
}

/*
    loadFont(const std::string& path):
    - Params:   const std::string& path
    - Returns:  Font
    - Desc:     Loads a TTF by path, cached so the same file is read once. A
                missing or invalid file falls back to the embedded face and is
                cached under the requested path too, so a bad path warns once
                rather than retrying every frame.
*/
Font Renderer::loadFont(const std::string& path) {
    auto& impl = *m_impl;

    /* Either face that ships inside the binary, cached under its own key. */
    auto loadBuiltIn = [&](const std::vector<uint8_t>& bytes,
                           const char* cacheKey,
                           const char* what) -> Font {
        auto itEmbedded = impl.fontByPath.find(cacheKey);
        if (itEmbedded != impl.fontByPath.end()) {
            Font f; f.id = itEmbedded->second; return f;
        }

        std::vector<uint8_t> ttf(bytes.begin(), bytes.end());
        stbtt_fontinfo probe{};
        if (!stbtt_InitFont(&probe, ttf.data(),
                           stbtt_GetFontOffsetForIndex(ttf.data(), 0))) {
            std::fprintf(stderr, "[UILO] loadFont: embedded %s font is invalid\n", what);
            return Font{};
        }

        Impl::FontRecord rec;
        rec.ttfData = std::move(ttf);
        uint32_t id = (uint32_t)impl.fonts.size();
        impl.fonts.push_back(std::move(rec));
        impl.fontByPath.emplace(cacheKey, id);

        Font f; f.id = id; return f;
    };

    auto loadEmbeddedFallback = [&]() -> Font {
        return loadBuiltIn(EMBEDDED_FONT, kEmbeddedFontCacheKey, "fallback");
    };

    if (path.empty()) {
        return loadEmbeddedFallback();
    }

    /* The reserved name Resources::fonts::mono resolves to. It is not a file,
       so it must be answered before anything tries to read it off disk. */
    if (path == kEmbeddedMonoFontPath) {
        Font f = loadBuiltIn(EMBEDDED_MONO_FONT, kEmbeddedMonoFontCacheKey, "monospaced");
        return f.valid() ? f : loadEmbeddedFallback();
    }

    auto it = impl.fontByPath.find(path);
    if (it != impl.fontByPath.end()) {
        Font f; f.id = it->second; return f;
    }

    std::vector<uint8_t> ttf;
    if (!readFile(path.c_str(), ttf)) {
        std::fprintf(stderr, "[UILO] loadFont: failed to read '%s', using embedded fallback\n", path.c_str());
        Font f = loadEmbeddedFallback();
        if (f.valid()) impl.fontByPath.emplace(path, f.id);
        return f;
    }

    /* Validate */
    stbtt_fontinfo probe{};
    if (!stbtt_InitFont(&probe, ttf.data(),
                       stbtt_GetFontOffsetForIndex(ttf.data(), 0))) {
        std::fprintf(stderr, "[UILO] loadFont: invalid TTF '%s', using embedded fallback\n", path.c_str());
        Font f = loadEmbeddedFallback();
        if (f.valid()) impl.fontByPath.emplace(path, f.id);
        return f;
    }
    Impl::FontRecord rec;
    rec.ttfData = std::move(ttf);
    uint32_t id = (uint32_t)impl.fonts.size();
    impl.fonts.push_back(std::move(rec));
    impl.fontByPath.emplace(path, id);
    Font f; f.id = id; return f;
}

/*
    measureText(const std::string& utf8, const Font& font, float sizePx):
    - Params:   const std::string& utf8, const Font& font, float sizePx
    - Returns:  TextMetrics
    - Desc:     Measures a string's bounding box and vertical metrics at a size,
                walking the same glyph advances the draw uses so the two always
                agree.
*/
TextMetrics Renderer::measureText(const std::string& utf8, const Font& font, float sizePx) {
    TextMetrics m{};
    if (!font.valid()) return m;
    auto& impl = *m_impl;
    FontFace* face = impl.getFace(font.id, sizePx);
    if (!face) return m;

    m.ascent   = face->ascent;
    m.descent  = face->descent;
    m.lineGap  = face->lineGap;

    float x = 0.f;
    float maxX = 0.f;
    int   lines = 1;
    const char* s = utf8.data();
    size_t left = utf8.size();
    uint32_t cp = 0;
    while (left > 0) {
        int n = utf8Decode(s, left, &cp);
        if (n <= 0) break;
        s += n; left -= n;
        if (cp == '\n') {
            if (x > maxX) maxX = x;
            x = 0.f;
            ++lines;
            continue;
        }
        const Glyph* g = impl.getGlyph(*face, cp);
        if (g) x += g->xadvance;
    }
    if (x > maxX) maxX = x;
    m.size.x = maxX;
    m.size.y = m.lineHeight() * (float)lines - m.lineGap;
    return m;
}

/*
    charPositions(const std::string& utf8, const Font& font, float sizePx):
    - Params:   const std::string& utf8, const Font& font, float sizePx
    - Returns:  std::vector<Vec2f>
    - Desc:     The position of every codepoint boundary in a string, N+1 of
                them so the trailing cursor slot has one. This is what the
                Textbox maps a caret and a hit test through.
*/
std::vector<Vec2f> Renderer::charPositions(const std::string& utf8,
                                            const Font& font, float sizePx) {
    std::vector<Vec2f> out;
    out.push_back({0.f, 0.f});
    if (!font.valid()) return out;
    auto& impl = *m_impl;
    FontFace* face = impl.getFace(font.id, sizePx);
    if (!face) return out;

    const float lh = face->ascent + face->descent + face->lineGap;
    float x = 0.f;
    float y = 0.f;

    const char* s = utf8.data();
    size_t left = utf8.size();
    uint32_t cp = 0;
    while (left > 0) {
        int n = utf8Decode(s, left, &cp);
        if (n <= 0) break;
        s += n; left -= n;
        if (cp == '\n') {
            x = 0.f;
            y += lh;
        } else if (cp == '\r') {
            /* skip; matches drawText */
        } else {
            const Glyph* g = impl.getGlyph(*face, cp);
            if (g) x += g->xadvance;
        }
        out.push_back({x, y});
    }
    return out;
}

/*
    drawText(...):
    - Params:   const std::string& utf8, Vec2f position, const Font& font, float
                sizePx, Color color, TextStyle style
    - Returns:  none
    - Desc:     Draws a UTF-8 string, one textured quad per glyph, batched into
                a single submit. Bold re-emits each glyph nudged sideways and
                italic shears the quad about the baseline, both synthetic so any
                face can be styled without a second font file, and neither
                changes the advances -- so a styled run measures and wraps
                exactly like a plain one.
*/
void Renderer::drawText(const std::string& utf8, Vec2f position,
                         const Font& font, float sizePx, Color color,
                         TextStyle style) {
    if (!m_initialised || !font.valid() || utf8.empty()) return;
    auto& impl = *m_impl;
    if (impl.scissorEmpty()) return;

    FontFace* face = impl.getFace(font.id, sizePx);
    if (!face || face->atlas == valo::EMPTY_TEXTURE) return;

    /* Synthetic styling. */
    const int   passes = style.bold ? 2 : 1;
    const float boldDx = style.bold ? std::max(1.f, sizePx * 0.04f) : 0.f;
    const float slant  = style.italic ? 0.21f : 0.f;   /* ~12 degrees */

    const float invW = 1.f / (float)face->atlasW;
    const float invH = 1.f / (float)face->atlasH;

    float penX = position.x;
    float penY = position.y + face->ascent;   /* baseline */
    bool  open = false;

    const char* s = utf8.data();
    size_t left = utf8.size();
    uint32_t cp = 0;
    while (left > 0) {
        int n = utf8Decode(s, left, &cp);
        if (n <= 0) break;
        s += n; left -= n;
        if (cp == '\n') {
            penX  = position.x;
            penY += face->ascent + face->descent + face->lineGap;
            continue;
        }
        if (cp == '\r') continue;

        const Glyph* g = impl.getGlyph(*face, cp);
        if (!g) continue;

        if (g->w > 0 && g->h > 0) {
            /* Opened on the first visible glyph, so whitespace-only text never
               breaks the batch it lands in. */
            if (!open) {
                if (!impl.beginShape(Impl::ModeText, face->atlas)) return;
                open = true;
            }
            float gx = std::floor(penX + g->xoff + 0.5f);
            float gy = std::floor(penY + g->yoff + 0.5f);
            float gw = (float)g->w;
            float gh = (float)g->h;
            float u0 = g->x * invW;
            float v0 = g->y * invH;
            float u1 = (g->x + g->w) * invW;
            float v1 = (g->y + g->h) * invH;

            for (int pass = 0; pass < passes; ++pass) {
                const float dx = (float)pass * boldDx;

                float p0x = gx + dx,      p0y = gy;
                float p1x = gx + gw + dx, p1y = gy;
                float p2x = gx + gw + dx, p2y = gy + gh;
                float p3x = gx + dx,      p3y = gy + gh;

                /* Shear about the baseline, so the glyph leans right above it. */
                if (slant != 0.f) {
                    p0x += (penY - p0y) * slant;
                    p1x += (penY - p1y) * slant;
                    p2x += (penY - p2y) * slant;
                    p3x += (penY - p3y) * slant;
                }

                impl.rotPt(p0x, p0y); impl.rotPt(p1x, p1y);
                impl.rotPt(p2x, p2y); impl.rotPt(p3x, p3y);
                const uint32_t base = impl.pushVertex(p0x, p0y, color, u0, v0);
                impl.pushVertex(p1x, p1y, color, u1, v0);
                impl.pushVertex(p2x, p2y, color, u1, v1);
                impl.pushVertex(p3x, p3y, color, u0, v1);
                impl.pushIndex(base);
                impl.pushIndex(base + 1);
                impl.pushIndex(base + 2);
                impl.pushIndex(base);
                impl.pushIndex(base + 2);
                impl.pushIndex(base + 3);
            }
        }
        penX += g->xadvance;
    }
}

} // namespace uilo
