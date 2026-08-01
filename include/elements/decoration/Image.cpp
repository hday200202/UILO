#include "Image.hpp"
#include "../../UILO.hpp"
#include "../../renderer/Renderer.hpp"

namespace uilo {

/*
    Image(Modifier modifier, ImageOptions options, const std::string& name):
    - Params:   Modifier modifier, ImageOptions options, const std::string& name
    - Returns:  Image
    - Desc:     Constructs an image element from a modifier and its options. The
                texture is not loaded here: there is no renderer until the
                element is bound to a UILO, so that waits for the first update.
*/
Image::Image(
    Modifier modifier,
    ImageOptions options,
    const std::string& name
) : m_options(options) {
    m_modifier = modifier;
    m_name     = name;
    m_type     = ElementType::Image;
}


/*
    ~Image():
    - Params:   none
    - Returns:  none
    - Desc:     Releases the private texture a pixel write may have created. A
                path-cached texture belongs to the Renderer and is left alone.
*/
Image::~Image() {
    releaseOwnedTexture();
}


/*
    init():
    - Params:   none
    - Returns:  void
    - Desc:     Loads the texture, once, on the first update that has a UILO to
                load it through. With an aspect lock set, the element's declared
                size is also rewritten here from the source dimensions, so the
                Modifier itself carries the derived axis and the parent reserves
                the right amount of space for it. A pixel-sized axis is required
                for that: a percent one has no fixed number to derive from and
                is left to the layout.
*/
void Image::init() {
    if (m_loaded || !m_uiloRef || m_options.getPath().empty()) return;

    Texture tex = m_uiloRef->getRenderer().loadTexture(m_options.getPath());
    if (!tex.valid()) return;

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


/*
    rebuildTexture():
    - Params:   none
    - Returns:  void
    - Desc:     Drops everything derived from the old file -- the private
                texture if there was one, the CPU pixel buffer, and the loaded
                state -- and reloads from the current path. Called when the
                options are replaced, since the path may have changed.
*/
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


/*
    isLoaded():
    - Params:   none
    - Returns:  bool
    - Desc:     Whether the texture loaded. False until the first update after
                the element is bound to a UILO, and permanently false when the
                file could not be read.
*/
bool Image::isLoaded() const { return m_loaded; }


/*
    releaseOwnedTexture():
    - Params:   none
    - Returns:  void
    - Desc:     Destroys the texture only when this Image owns it. Cached
                textures belong to the Renderer and are shared with every other
                Image naming the same file, so only a private copy-on-write
                texture is ours to destroy.
*/
void Image::releaseOwnedTexture() {
    if (!m_ownsTexture) return;

    if (m_uiloRef) {
        Texture tex;
        tex.handle = m_textureHandle;
        m_uiloRef->getRenderer().destroyTexture(tex);
    }
    m_textureHandle = 0xFFFFu;
    m_ownsTexture   = false;
    m_loaded        = false;
}


/*
    ensurePixels():
    - Params:   none
    - Returns:  bool -- true when a CPU-side pixel buffer is available
    - Desc:     Decodes the source file into a CPU-side RGBA8 buffer the first
                time pixel access needs it. Already-decoded buffers are kept, so
                repeated reads cost nothing after the first.
*/
bool Image::ensurePixels() const {
    if (!m_pixels.empty()) return true;
    if (!m_uiloRef || m_options.getPath().empty()) return false;

    return m_uiloRef->getRenderer().loadImagePixels(
        m_options.getPath(), m_pixels, m_pixelsWidth, m_pixelsHeight);
}


/*
    getPixel(const uint32_t x, const uint32_t y):
    - Params:   const uint32_t x, const uint32_t y
    - Returns:  Color -- transparent when unavailable or out of bounds
    - Desc:     Reads one texel from the source image, decoding it on first use.
                Coordinates are in source texels and are unaffected by on-screen
                size, flips or recolor.
*/
Color Image::getPixel(const uint32_t x, const uint32_t y) const {
    if (!ensurePixels()) return Color{0, 0, 0, 0};
    if (x >= m_pixelsWidth || y >= m_pixelsHeight) return Color{0, 0, 0, 0};

    const uint8_t* p = &m_pixels[((size_t)y * m_pixelsWidth + x) * 4];
    return Color{p[0], p[1], p[2], p[3]};
}


/*
    setPixel(const uint32_t x, const uint32_t y, const Color& color):
    - Params:   const uint32_t x, const uint32_t y, const Color& color
    - Returns:  void
    - Desc:     Writes one texel into the CPU-side buffer and marks it for
                upload. The write lands on the GPU at the next render rather
                than immediately, so setting many pixels in a frame costs one
                upload. Out-of-bounds writes are ignored.
*/
void Image::setPixel(const uint32_t x, const uint32_t y, const Color& color) {
    if (!ensurePixels()) return;
    if (x >= m_pixelsWidth || y >= m_pixelsHeight) return;

    uint8_t* p = &m_pixels[((size_t)y * m_pixelsWidth + x) * 4];
    p[0] = color.r; p[1] = color.g; p[2] = color.b; p[3] = color.a;
    m_pixelsDirty = true;
}


/*
    syncPixels():
    - Params:   none
    - Returns:  void
    - Desc:     Uploads pending pixel writes, detaching onto a private texture
                the first time. The path-cached texture is shared with any other
                Image using the same file and is immutable in bgfx anyway, so
                the first write creates a private mutable texture of its own and
                this Image takes ownership of it from then on.
*/
void Image::syncPixels() {
    if (!m_pixelsDirty || m_pixels.empty() || !m_uiloRef) return;

    Renderer& renderer = m_uiloRef->getRenderer();
    if (!m_ownsTexture) {
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


/*
    update(Rectf& parentBounds, float dt):
    - Params:   Rectf& parentBounds, float dt
    - Returns:  void
    - Desc:     Loads the texture if that has not happened yet, resolves the
                element's bounds, and then overrides one axis from the source
                aspect ratio when an aspect lock is set. The override happens
                after resize rather than instead of it, so the locked axis is
                measured against whatever the layout actually gave the other
                one.
*/
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


/*
    render():
    - Params:   none
    - Returns:  void
    - Desc:     Uploads any pending pixel writes and draws the textured quad,
                with the flips and elliptical clip the options ask for. The
                upload runs before the loaded check on purpose: a setPixel made
                before the first frame creates the texture rather than needing
                one to already exist. The tint is white unless recolor is on, so
                an ordinary image is drawn at its own colours.
*/
void Image::render() {
    if (!m_modifier.getVisible()) { m_dirty = false; return; }
    m_dirty = false;

    syncPixels();
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
