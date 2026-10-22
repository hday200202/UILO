#include "RendererImpl.hpp"

/* Declarations only: VALO compiles stb_image's implementation into itself
   (render/software/RenderTarget.cpp), and a second copy would collide. */
#include "stb_image.h"

namespace uilo {

/*
    loadTexture(const std::string& path):
    - Params:   const std::string& path
    - Returns:  Texture
    - Desc:     Decodes an image file into a GPU texture, cached by path so the
                same file is decoded once. A file that fails to load is cached as
                an invalid texture too, so a bad path is reported once rather
                than retried every frame.
*/
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

    /* stb's req_comp=4 bytes are R,G,B,A in memory, which is VALO's texel
       layout. No mips and no sRGB decode, so an image samples exactly as it did
       under bgfx. */
    const valo::TextureID id = m_initialised
        ? impl.valo.createTexture((uint32_t)w, (uint32_t)h,
                                  reinterpret_cast<const uint32_t*>(pixels), false, false)
        : valo::EMPTY_TEXTURE;
    stbi_image_free(pixels);

    Texture tex;
    if (id != valo::EMPTY_TEXTURE && id < UINT16_MAX) {
        tex.handle = static_cast<uint16_t>(id);
        tex.width  = (uint16_t)w;
        tex.height = (uint16_t)h;
    }
    impl.textureCache.emplace(path, tex);
    return tex;
}

/*
    loadImagePixels(...):
    - Params:   const std::string& path, std::vector<uint8_t>& outRgba,
                uint32_t& outWidth, uint32_t& outHeight
    - Returns:  bool
    - Desc:     Decodes an image file into a CPU-side RGBA8 buffer without
                creating a texture, which is what backs Image's get and set
                pixel access.
*/
bool Renderer::loadImagePixels(const std::string& path, std::vector<uint8_t>& outRgba,
                               uint32_t& outWidth, uint32_t& outHeight) {
    int w = 0, h = 0, comp = 0;
    stbi_uc* pixels = stbi_load(path.c_str(), &w, &h, &comp, 4);
    if (!pixels) {
        std::fprintf(stderr, "[UILO] loadImagePixels: failed to load '%s': %s\n",
                     path.c_str(), stbi_failure_reason());
        return false;
    }
    outRgba.assign(pixels, pixels + (size_t)w * (size_t)h * 4);
    stbi_image_free(pixels);
    outWidth  = (uint32_t)w;
    outHeight = (uint32_t)h;
    return true;
}

/*
    createTexture(uint16_t width, uint16_t height):
    - Params:   uint16_t width, uint16_t height
    - Returns:  Texture
    - Desc:     Creates an empty mutable RGBA8 texture, for content the
                application updates itself rather than loading from a file.
                Created without mips, since every update would otherwise rebuild
                the whole chain.
*/
Texture Renderer::createTexture(uint16_t width, uint16_t height) {
    if (width == 0 || height == 0 || !m_initialised) return Texture{};
    std::vector<uint32_t> blank((size_t)width * (size_t)height, 0u);
    const valo::TextureID id = m_impl->valo.createTexture(width, height, blank.data(), false, false);
    Texture tex;
    if (id == valo::EMPTY_TEXTURE || id >= UINT16_MAX) return tex;
    tex.handle = static_cast<uint16_t>(id);
    tex.width  = width;
    tex.height = height;
    return tex;
}

/*
    updateTexture(const Texture& tex, const uint8_t* rgba):
    - Params:   const Texture& tex, const uint8_t* rgba
    - Returns:  none
    - Desc:     Uploads a full RGBA8 buffer into a texture created by
                createTexture. Takes effect for the frame being built.
*/
void Renderer::updateTexture(const Texture& tex, const uint8_t* rgba) {
    if (!tex.valid() || !rgba || !m_initialised) return;
    /* VALO wants whole texels; copying keeps a caller's byte buffer from having
       to be 4-byte aligned. */
    std::vector<uint32_t> texels((size_t)tex.width * (size_t)tex.height);
    std::memcpy(texels.data(), rgba, texels.size() * sizeof(uint32_t));
    m_impl->valo.updateTexture(tex.handle, texels.data());
}

/*
    destroyTexture(Texture& tex):
    - Params:   Texture& tex
    - Returns:  none
    - Desc:     Destroys a texture and invalidates the handle. Only textures the
                caller owns should be passed: a path-cached one belongs to the
                renderer and is shared. VALO defers the release until no frame
                in flight still samples it.
*/
void Renderer::destroyTexture(Texture& tex) {
    if (!tex.valid()) return;
    if (m_initialised) m_impl->valo.destroyTexture(tex.handle);
    tex.handle = UINT16_MAX;
    /* (We don't bother removing from cache; it'll be cleared on shutdown.) */
}

/*
    drawImage(...):
    - Params:   const Rectf& dst, const Texture& tex, Color tint, Rectf uv, bool
                flipH, bool flipV, bool clipEllipse
    - Returns:  none
    - Desc:     Draws a texture into a rectangle, with a tint, a source sub-
                rectangle, optional horizontal and vertical flips, and an
                optional elliptical mask for a round avatar.
*/
void Renderer::drawImage(const Rectf& dst, const Texture& tex,
                          Color tint, Rectf uv,
                          bool flipH, bool flipV,
                          bool clipEllipse) {
    if (!tex.valid() || !m_initialised) return;
    auto& impl = *m_impl;

    float x = dst.position.x, y = dst.position.y;
    float w = dst.size.x,     h = dst.size.y;

    float u0 = uv.position.x, v0 = uv.position.y;
    float u1 = uv.position.x + uv.size.x;
    float v1 = uv.position.y + uv.size.y;
    if (flipH) std::swap(u0, u1);
    if (flipV) std::swap(v0, v1);

    /* The ellipse mask antialiases over fwidth of its own distance, which is
       known from how many texture units one pixel spans on each axis. */
    const float flags[4] = {
        clipEllipse ? 1.f : 0.f,
        w != 0.f ? std::abs(u1 - u0) / std::abs(w) : 0.f,
        h != 0.f ? std::abs(v1 - v0) / std::abs(h) : 0.f,
        0.f
    };
    if (!impl.beginShape(Impl::ModeImage, tex.handle, flags)) return;

    float x0 = x,     y0 = y;
    float x1 = x + w, y1 = y;
    float x2 = x + w, y2 = y + h;
    float x3 = x,     y3 = y + h;
    impl.rotPt(x0, y0); impl.rotPt(x1, y1);
    impl.rotPt(x2, y2); impl.rotPt(x3, y3);

    const uint32_t b = impl.pushVertex(x0, y0, tint, u0, v0);
    impl.pushVertex(x1, y1, tint, u1, v0);
    impl.pushVertex(x2, y2, tint, u1, v1);
    impl.pushVertex(x3, y3, tint, u0, v1);
    impl.pushIndex(b + 0); impl.pushIndex(b + 1); impl.pushIndex(b + 2);
    impl.pushIndex(b + 0); impl.pushIndex(b + 2); impl.pushIndex(b + 3);
}

} // namespace uilo
