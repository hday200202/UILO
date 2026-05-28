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
    m_loaded = false;
    m_textureHandle = 0xFFFFu;
    init();
}

bool Image::isLoaded() const { return m_loaded; }

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
