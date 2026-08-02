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
                metrics, scales them, and allocates a blank single-channel atlas
                the glyphs are packed into on demand.
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

    /* Allocate R8 texture, blank. */
    face.atlas = bgfx::createTexture2D(
        (uint16_t)kAtlasW, (uint16_t)kAtlasH, false, 1,
        bgfx::TextureFormat::R8,
        BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP,
        nullptr);
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

    /* Upload subregion */
    const bgfx::Memory* mem = bgfx::copy(bmp.data(), (uint32_t)bmp.size());
    bgfx::updateTexture2D(face.atlas, 0, 0,
                          (uint16_t)face.penX, (uint16_t)face.penY,
                          (uint16_t)gw, (uint16_t)gh,
                          mem, (uint16_t)gw);

    g.x = (uint16_t)face.penX;
    g.y = (uint16_t)face.penY;
    g.w = (uint16_t)gw;
    g.h = (uint16_t)gh;
    face.penX += gw + pad;
    if (gh > face.rowH) face.rowH = gh;

    return &face.glyphs.emplace(codepoint, g).first->second;
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
    if (!font.valid() || utf8.empty()) return;
    auto& impl = *m_impl;
    impl.flushSolidBatch();
    if (!bgfx::isValid(impl.textProgram) || scissorEmpty(impl)) return;

    FontFace* face = impl.getFace(font.id, sizePx);
    if (!face || !bgfx::isValid(face->atlas)) return;

    /* Synthetic styling. */
    const int   passes = style.bold ? 2 : 1;
    const float boldDx = style.bold ? std::max(1.f, sizePx * 0.04f) : 0.f;
    const float slant  = style.italic ? 0.21f : 0.f;   /* ~12 degrees */

    /* Single pass: allocate transient space for the worst case (one quad per
       UTF-8 byte, an upper bound on codepoints) and submit only the range. */
    const uint32_t maxQuads =
        (uint32_t)std::min<size_t>(utf8.size() * (size_t)passes, 65532u / 4u);
    bgfx::TransientVertexBuffer tvb;
    bgfx::TransientIndexBuffer  tib;
    if (!bgfx::allocTransientBuffers(&tvb, impl.texLayout, maxQuads * 4,
                                     &tib, maxQuads * 6))
        return;

    auto* verts = reinterpret_cast<PosColorUvVertex*>(tvb.data);
    auto* idx   = reinterpret_cast<uint16_t*>(tib.data);

    uint32_t col = packColor(color);
    const float invW = 1.f / (float)face->atlasW;
    const float invH = 1.f / (float)face->atlasH;

    float penX = position.x;
    float penY = position.y + face->ascent;   /* baseline */
    uint32_t vi = 0, ii = 0;

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
            float gx = std::floor(penX + g->xoff + 0.5f);
            float gy = std::floor(penY + g->yoff + 0.5f);
            float gw = (float)g->w;
            float gh = (float)g->h;
            float u0 = g->x * invW;
            float v0 = g->y * invH;
            float u1 = (g->x + g->w) * invW;
            float v1 = (g->y + g->h) * invH;

            for (int pass = 0; pass < passes; ++pass) {
                if (vi + 4 > maxQuads * 4) break;
                const float dx = (float)pass * boldDx;

                uint16_t base = (uint16_t)vi;
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
                verts[vi++] = {p0x, p0y, col, u0, v0};
                verts[vi++] = {p1x, p1y, col, u1, v0};
                verts[vi++] = {p2x, p2y, col, u1, v1};
                verts[vi++] = {p3x, p3y, col, u0, v1};
                idx[ii++] = base;
                idx[ii++] = (uint16_t)(base + 1);
                idx[ii++] = (uint16_t)(base + 2);
                idx[ii++] = base;
                idx[ii++] = (uint16_t)(base + 2);
                idx[ii++] = (uint16_t)(base + 3);
            }
        }
        penX += g->xadvance;
    }
    if (ii == 0) return;   /* whitespace-only: nothing to submit */

    bgfx::setTexture(0, impl.s_texColor, face->atlas);
    bgfx::setVertexBuffer(0, &tvb, 0, vi);
    bgfx::setIndexBuffer(&tib, 0, ii);
    bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A |
                   BGFX_STATE_BLEND_ALPHA);
    applyScissor(impl);
    bgfx::submit(currentViewId(), impl.textProgram);
}

} // namespace uilo
