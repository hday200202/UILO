#include "Text.hpp"
#include "../../UILO.hpp"
#include "../../utils/Alignment.hpp"
#include "../../renderer/Renderer.hpp"

#include <sstream>

namespace uilo {

Text::Text(
    Modifier modifier,
    TextOptions options,
    const std::string& name
) : m_options(options), m_content(options.getContent()),
  m_charSize(options.hasCharSize() ? options.getCharSize() : 30) {
    m_modifier = modifier;
    m_name     = name;
    m_type     = ElementType::Text;
}

void Text::init() {
    if (m_loaded || !m_uiloRef) return;
    if (m_options.getFontPath().empty()) return;
    Font f = m_uiloRef->getRenderer().loadFont(m_options.getFontPath());
    if (f.valid()) {
        m_fontId = f.id;
        m_loaded = true;
        m_wrappedContent = m_content;
    }
}

std::string Text::wrapContent(float maxWidth) const {
    if (!m_uiloRef || !m_loaded || maxWidth <= 0.f) return m_content;
    auto& renderer = m_uiloRef->getRenderer();
    Font f; f.id = m_fontId;
    const float scale = m_uiloRef->getScale();
    const float pxH   = (float)m_charSize * scale;

    std::string result;
    result.reserve(m_content.size() + 16);

    std::stringstream ss(m_content);
    std::string word;
    std::string line;
    while (std::getline(ss, word, ' ')) {
        std::string candidate = line.empty() ? word : (line + " " + word);
        TextMetrics m = renderer.measureText(candidate, f, pxH);
        if (m.size.x > maxWidth && !line.empty()) {
            result += line; result += '\n';
            line = word;
        } else {
            line = candidate;
        }
    }
    result += line;
    return result;
}

void Text::rebuildText() {
    if (m_options.getWrap() && m_lastWrapWidth > 0.f) {
        m_wrappedContent = wrapContent(m_lastWrapWidth);
    } else {
        m_wrappedContent = m_content;
    }
    m_cachedMetricsValid = false;
}

bool Text::isLoaded() const { return m_loaded; }

void Text::setString(const std::string& content) {
    if (content == m_content) return;
    m_content = content;
    m_wrappedContent = content;
    m_dirty = true;
    m_cachedMetricsValid = false;
    if (m_loaded) rebuildText();
}

void Text::update(Rectf& parentBounds, float dt) {
    (void)dt;
    if (!m_loaded) init();
    if (!m_loaded) return;

    resize(parentBounds);

    float scale = m_uiloRef ? m_uiloRef->getScale() : 1.f;
    bool needRebuild = false;

    if (!m_options.hasCharSize()) {
        const unsigned int autoCs = std::max(1u,
            static_cast<unsigned int>(m_bounds.size.y * 0.6f / scale));
        if (autoCs != m_charSize) {
            m_charSize   = autoCs;
            needRebuild  = true;
        }
    }
    if (m_options.getWrap() && m_bounds.size.x != m_lastWrapWidth) {
        m_lastWrapWidth = m_bounds.size.x;
        needRebuild = true;
    }
    if (scale != m_lastScale) {
        m_lastScale = scale;
        needRebuild = true;
    }
    if (needRebuild) rebuildText();
}

void Text::render() {
    m_dirty = false;
    if (!m_loaded || !m_uiloRef) return;
    if (m_wrappedContent.empty()) return;

    auto& renderer = m_uiloRef->getRenderer();
    Font f; f.id = m_fontId;
    const float scale = m_uiloRef->getScale();
    const float pxH   = (float)m_charSize * scale;

    if (!m_cachedMetricsValid) {
        m_cachedMetrics      = renderer.measureText(m_wrappedContent, f, pxH);
        m_cachedMetricsValid = true;
    }
    const TextMetrics& m = m_cachedMetrics;

    Vec2f pos = m_bounds.position;
    switch (m_options.getTextAlignX()) {
        case Align::CenterX: pos.x += (m_bounds.size.x - m.size.x) * 0.5f; break;
        case Align::Right:   pos.x += (m_bounds.size.x - m.size.x);        break;
        default: break;
    }
    switch (m_options.getTextAlignY()) {
        case Align::CenterY: pos.y += (m_bounds.size.y - m.size.y) * 0.5f; break;
        case Align::Bottom:  pos.y += (m_bounds.size.y - m.size.y);        break;
        default: break;
    }

    const Color textColor = m_uiloRef
        ? m_uiloRef->getPalette().resolve(m_options.getColorRole(), m_options.getColor())
        : m_options.getColor();
    renderer.drawText(m_wrappedContent, pos, f, pxH, textColor);
}

} // namespace uilo
