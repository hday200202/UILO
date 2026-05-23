#include "Image.hpp"

namespace uilo {

Image::Image(
    Modifier modifier,
    const std::string& path,
    ImageOptions options,
    const std::string& name
) : m_options(options) {
    m_modifier = modifier;
    m_name     = name;
    m_type     = ElementType::Image;

    if (!m_originalImage.loadFromFile(path)) return;
    init();
}

Image::Image(
    Modifier modifier,
    sf::Image sourceImg,
    ImageOptions options,
    const std::string& name
) : m_originalImage(std::move(sourceImg)), m_options(options) {
    m_modifier = modifier;
    m_name     = name;
    m_type     = ElementType::Image;

    init();
}

void Image::init() {
    if (hasOption(m_options, ImageOptions::Recolor) ||
        hasOption(m_options, ImageOptions::ClipEllipse)) {
        m_lastRecolor = m_modifier.getColor();
        rebuildTexture();
    } else {
        if (!m_texture.loadFromImage(m_originalImage)) return;
    }

    m_sprite.emplace(m_texture);
    m_loaded = true;
}

void Image::rebuildTexture() {
    sf::Image working = m_originalImage;
    auto size = working.getSize();

    if (hasOption(m_options, ImageOptions::Recolor)) {
        sf::Color tgt = m_modifier.getColor();
        for (unsigned y = 0; y < size.y; ++y) {
            for (unsigned x = 0; x < size.x; ++x) {
                sf::Color px = working.getPixel({x, y});
                float luma = 0.299f * (px.r / 255.f)
                           + 0.587f * (px.g / 255.f)
                           + 0.114f * (px.b / 255.f);
                float dark = 1.f - luma;
                px.r = tgt.r;
                px.g = tgt.g;
                px.b = tgt.b;
                px.a = static_cast<uint8_t>(px.a * dark);
                working.setPixel({x, y}, px);
            }
        }
    }

    if (hasOption(m_options, ImageOptions::ClipEllipse)) {
        float cx = size.x * 0.5f;
        float cy = size.y * 0.5f;
        for (unsigned y = 0; y < size.y; ++y) {
            for (unsigned x = 0; x < size.x; ++x) {
                float dx = (x + 0.5f - cx) / cx;
                float dy = (y + 0.5f - cy) / cy;
                if (dx * dx + dy * dy > 1.0f) {
                    sf::Color px = working.getPixel({x, y});
                    px.a = 0;
                    working.setPixel({x, y}, px);
                }
            }
        }
    }

    if (!m_texture.loadFromImage(working)) return;
}

bool Image::isLoaded() const { return m_loaded; }

void Image::update(sf::FloatRect& parentBounds, float dt) {
    (void)dt;
    if (!m_loaded) return;

    resize(parentBounds);

    auto texSize = m_texture.getSize();
    float op    = m_modifier.getOuterPadding();
    Align align = m_modifier.getAlign();

    if (hasOption(m_options, ImageOptions::LockAspectWidth)) {
        float aspect     = static_cast<float>(texSize.y) / static_cast<float>(texSize.x);
        m_bounds.size.y  = m_bounds.size.x * aspect;

        float innerTop = parentBounds.position.y + op;
        float innerH   = parentBounds.size.y - 2.f * op;
        if      (hasAlign(align, Align::Bottom)) m_bounds.position.y = innerTop + innerH - m_bounds.size.y;
        else if (hasAlign(align, Align::CenterY))   m_bounds.position.y = innerTop + (innerH - m_bounds.size.y) * 0.5f;
        else                                      m_bounds.position.y = innerTop;
    }
    else if (hasOption(m_options, ImageOptions::LockAspectHeight)) {
        float aspect     = static_cast<float>(texSize.x) / static_cast<float>(texSize.y);
        m_bounds.size.x  = m_bounds.size.y * aspect;

        float innerLeft = parentBounds.position.x + op;
        float innerW    = parentBounds.size.x - 2.f * op;
        if      (hasAlign(align, Align::Right)) m_bounds.position.x = innerLeft + innerW - m_bounds.size.x;
        else if (hasAlign(align, Align::CenterX))  m_bounds.position.x = innerLeft + (innerW - m_bounds.size.x) * 0.5f;
        else                                     m_bounds.position.x = innerLeft;
    }
}

void Image::render(sf::RenderTarget& target) {
    if (!m_loaded || !m_sprite) return;

    if (hasOption(m_options, ImageOptions::Recolor)) {
        sf::Color cur = m_modifier.getColor();
        if (cur != m_lastRecolor) {
            m_lastRecolor = cur;
            rebuildTexture();
        }
    }

    auto texSize = m_texture.getSize();
    float sx = m_bounds.size.x / static_cast<float>(texSize.x);
    float sy = m_bounds.size.y / static_cast<float>(texSize.y);

    m_sprite->setPosition(m_bounds.position);
    m_sprite->setScale({sx, sy});
    m_sprite->setColor(sf::Color::White);

    target.draw(*m_sprite);
}

} // namespace uilo
