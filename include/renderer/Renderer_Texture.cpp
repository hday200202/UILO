#include "RendererImpl.hpp"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO   // we'll feed a buffer; actually keep stdio for stbi_load
#undef  STBI_NO_STDIO
#include "stb_image.h"

namespace uilo {

namespace {
inline void applyScissor(const Renderer::Impl& impl) {
    if (impl.scissorTop > 0) {
        auto& sc = impl.scissorStack[impl.scissorTop - 1];
        bgfx::setScissor(sc.x, sc.y, sc.w, sc.h);
    }
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
                          bool flipH, bool flipV) {
    if (!tex.valid()) return;
    auto& impl = *m_impl;
    if (!bgfx::isValid(impl.texProgram) || scissorEmpty(impl)) return;

    float x = dst.position.x, y = dst.position.y;
    float w = dst.size.x,     h = dst.size.y;

    float u0 = uv.position.x, v0 = uv.position.y;
    float u1 = uv.position.x + uv.size.x;
    float v1 = uv.position.y + uv.size.y;
    if (flipH) std::swap(u0, u1);
    if (flipV) std::swap(v0, v1);

    uint32_t col = packColor(tint);

    PosColorUvVertex verts[4] = {
        {x,     y,     col, u0, v0},
        {x + w, y,     col, u1, v0},
        {x + w, y + h, col, u1, v1},
        {x,     y + h, col, u0, v1},
    };
    uint16_t idx[6] = {0,1,2, 0,2,3};

    bgfx::TransientVertexBuffer tvb;
    bgfx::TransientIndexBuffer  tib;
    if (!bgfx::allocTransientBuffers(&tvb, impl.texLayout, 4, &tib, 6)) return;
    std::memcpy(tvb.data, verts, sizeof(verts));
    std::memcpy(tib.data, idx,   sizeof(idx));

    bgfx::TextureHandle th{ tex.handle };
    bgfx::setTexture(0, impl.s_texColor, th);
    bgfx::setVertexBuffer(0, &tvb);
    bgfx::setIndexBuffer(&tib);
    bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A |
                   BGFX_STATE_BLEND_ALPHA);
    applyScissor(impl);
    bgfx::submit(currentViewId(), impl.texProgram);
}

} // namespace uilo
