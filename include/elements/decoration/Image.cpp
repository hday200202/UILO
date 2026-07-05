#include "Image.hpp"
#include "../../UILO.hpp"
#include "../../renderer/Renderer.hpp"

namespace uilo {

Image::Image(
    Modifier modifier,
    ImageOptions options,
    const std::string& name
) : m_options(options) {
    m_modifier = modifier;
    m_name     = name;
    m_type     = ElementType::Image;
}

void Image::init() {
    if (m_loaded || !m_uiloRef || m_options.getPath().empty()) return;
    Texture tex = m_uiloRef->getRenderer().loadTexture(m_options.getPath());
    if (tex.valid()) {
        m_textureHandle = tex.handle;
        m_textureWidth  = tex.width;
        m_textureHeight = tex.height;
        m_loaded = true;

        if (m_textureWidth > 0 && m_textureHeight > 0) {
            const float aspect = (float)m_textureWidth / (float)m_textureHeight;
            Dimension w = m_modifier.getWidth();
            Dimension h = m_modifier.getHeight();
            if (m_options.getLockAspectWidth() && !w.percent)
                m_modifier.setHeight(Dimension{ w.value / aspect, false });
            else if (m_options.getLockAspectHeight() && !h.percent)
                m_modifier.setWidth(Dimension{ h.value * aspect, false });
        }
    }
}

void Image::rebuildTexture() {
    releaseOwnedTexture();
    m_pixels.clear();
    m_pixelsWidth  = 0;
    m_pixelsHeight = 0;
    m_pixelsDirty  = false;
    m_loaded = false;
    m_textureHandle = 0xFFFFu;
    init();
}

bool Image::isLoaded() const { return m_loaded; }

Image::~Image() {
    releaseOwnedTexture();
}

void Image::releaseOwnedTexture() {
    if (!m_ownsTexture) return;
    // Cached textures belong to the Renderer; only a private copy-on-write
    // texture is ours to destroy.
    if (m_uiloRef) {
        Texture tex;
        tex.handle = m_textureHandle;
        m_uiloRef->getRenderer().destroyTexture(tex);
    }
    m_textureHandle = 0xFFFFu;
    m_ownsTexture   = false;
    m_loaded        = false;
}

bool Image::ensurePixels() const {
    if (!m_pixels.empty()) return true;
    if (!m_uiloRef || m_options.getPath().empty()) return false;
    return m_uiloRef->getRenderer().loadImagePixels(
        m_options.getPath(), m_pixels, m_pixelsWidth, m_pixelsHeight);
}

Color Image::getPixel(const uint32_t x, const uint32_t y) const {
    if (!ensurePixels()) return Color{0, 0, 0, 0};
    if (x >= m_pixelsWidth || y >= m_pixelsHeight) return Color{0, 0, 0, 0};
    const uint8_t* p = &m_pixels[((size_t)y * m_pixelsWidth + x) * 4];
    return Color{p[0], p[1], p[2], p[3]};
}

void Image::setPixel(const uint32_t x, const uint32_t y, const Color& color) {
    if (!ensurePixels()) return;
    if (x >= m_pixelsWidth || y >= m_pixelsHeight) return;
    uint8_t* p = &m_pixels[((size_t)y * m_pixelsWidth + x) * 4];
    p[0] = color.r; p[1] = color.g; p[2] = color.b; p[3] = color.a;
    m_pixelsDirty = true;
}

void Image::syncPixels() {
    if (!m_pixelsDirty || m_pixels.empty() || !m_uiloRef) return;
    Renderer& renderer = m_uiloRef->getRenderer();
    if (!m_ownsTexture) {
        // First write: detach from the path-cached texture (shared with any
        // other Image using the same file, and immutable in bgfx anyway)
        // onto a private mutable one.
        Texture own = renderer.createTexture(
            (uint16_t)m_pixelsWidth, (uint16_t)m_pixelsHeight);
        if (!own.valid()) return;
        m_textureHandle = own.handle;
        m_textureWidth  = m_pixelsWidth;
        m_textureHeight = m_pixelsHeight;
        m_ownsTexture   = true;
        m_loaded        = true;
    }
    Texture tex;
    tex.handle = m_textureHandle;
    tex.width  = (uint16_t)m_pixelsWidth;
    tex.height = (uint16_t)m_pixelsHeight;
    renderer.updateTexture(tex, m_pixels.data());
    m_pixelsDirty = false;
}


void Image::update(Rectf& parentBounds, float dt) {
    (void)dt;
    if (!m_loaded) init();
    resize(parentBounds);

    if (m_loaded && m_textureWidth > 0 && m_textureHeight > 0) {
        const float aspect = (float)m_textureWidth / (float)m_textureHeight;
        if (m_options.getLockAspectHeight()) {
            m_bounds.size.x = m_bounds.size.y * aspect;
        } else if (m_options.getLockAspectWidth()) {
            m_bounds.size.y = m_bounds.size.x / aspect;
        }
    }
}

void Image::render() {
    m_dirty = false;
    syncPixels();   // before the m_loaded check: a setPixel made before the
                    // first frame creates the texture rather than needing one
    if (!m_loaded || !m_uiloRef) return;
    Texture tex;
    tex.handle = m_textureHandle;
    tex.width  = (uint16_t)m_textureWidth;
    tex.height = (uint16_t)m_textureHeight;
    const Color literal = m_options.getRecolor() ? m_options.getColor() : Color::White;
    const Color tint = m_options.getRecolor()
        ? m_uiloRef->getPalette().resolve(m_options.getColorRole(), literal)
        : literal;
    m_uiloRef->getRenderer().drawImage(
        m_bounds, tex, tint, {{0.f, 0.f}, {1.f, 1.f}},
        m_options.getFlipH(), m_options.getFlipV(),
        m_options.getClipEllipse());
}

} // namespace uilo
