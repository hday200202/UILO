#include "Image.hpp"

namespace uilo {

Image::Image(
    Modifier modifier,
    const uint8_t* pixels,
    uint32_t width,
    uint32_t height,
    bool keepAspectRatio,
    const std::string& name
)
    : m_pixels(pixels, pixels + width * height * 4),
      m_nativeWidth(width), m_nativeHeight(height),
      m_keepAspectRatio(keepAspectRatio)
{
    m_modifier = modifier;
    m_type = ElementType::Image;
    m_name = name;
}

const std::vector<uint8_t>& Image::getPixels() const { return m_pixels; }
uint32_t Image::getNativeWidth() const { return m_nativeWidth; }
uint32_t Image::getNativeHeight() const { return m_nativeHeight; }

void Image::setKeepAspectRatio(bool keep) { m_keepAspectRatio = keep; }
bool Image::getKeepAspectRatio() const { return m_keepAspectRatio; }

void Image::recolor(const Color& keyColor, const Color& newColor) {
    for (size_t i = 0; i + 3 < m_pixels.size(); i += 4) {
        if (m_pixels[i] == keyColor.r &&
            m_pixels[i + 1] == keyColor.g &&
            m_pixels[i + 2] == keyColor.b &&
            m_pixels[i + 3] == keyColor.a)
        {
            m_pixels[i] = newColor.r;
            m_pixels[i + 1] = newColor.g;
            m_pixels[i + 2] = newColor.b;
            m_pixels[i + 3] = newColor.a;
        }
    }
}

void Image::update(Bounds& parentBounds, float) {
    resize(parentBounds);

    if (m_keepAspectRatio && m_nativeWidth > 0 && m_nativeHeight > 0) {
        float scaleX = m_bounds.size.x / m_nativeWidth;
        float scaleY = m_bounds.size.y / m_nativeHeight;
        float scale = (scaleX < scaleY) ? scaleX : scaleY;
        m_bounds.size.x = m_nativeWidth * scale;
        m_bounds.size.y = m_nativeHeight * scale;
    }
}

void Image::render(Renderer& renderer) {
    renderer.drawImage(*this);
}

} // namespace uilo
