#include "Text.hpp"
#include "../../UILO.hpp"
#include "../../utils/Alignment.hpp"

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

    if (options.getFontRef()) {
        m_fontPtr = options.getFontRef();
        init();
    } else if (!options.getFontPath().empty()) {
        if (!m_ownedFont.openFromFile(options.getFontPath())) return;
        m_fontPtr = &m_ownedFont;
        init();
    }
}

void Text::init() {
    m_lastColor = m_options.getColor();
    rebuildText();
    m_loaded = true;
}

std::string Text::wrapContent(float maxWidth) const {
    if (maxWidth <= 0.f) return m_content;

    unsigned int cs = static_cast<unsigned int>(std::round(m_charSize * (m_uiloRef ? m_uiloRef->getScale() : 1.f)));
    sf::Text probe(*m_fontPtr, "", cs);
    std::string result;

    std::istringstream paragraphs(m_content);
    std::string paragraph;
    bool firstParagraph = true;

    while (std::getline(paragraphs, paragraph)) {
        if (!firstParagraph) result += '\n';
        firstParagraph = false;

        std::istringstream words(paragraph);
        std::string word, line;

        while (words >> word) {
            std::string test = line.empty() ? word : line + ' ' + word;
            probe.setString(test);
            if (!line.empty() && probe.getLocalBounds().size.x > maxWidth) {
                result += line + '\n';
                line = word;
            } else {
                line = test;
            }
        }
        result += line;
    }

    return result;
}

void Text::rebuildText() {
    if (!m_fontPtr) return;

    unsigned int cs = static_cast<unsigned int>(std::round(m_charSize * (m_uiloRef ? m_uiloRef->getScale() : 1.f)));
    float wrapWidth = m_bounds.size.x;
    std::string str = (m_options.getWrap() && wrapWidth > 0.f)
                    ? wrapContent(wrapWidth)
                    : m_content;

    m_text.emplace(*m_fontPtr, str, cs);

    std::uint32_t style = 0;
    if (m_options.getBold())          style |= static_cast<std::uint32_t>(sf::Text::Style::Bold);
    if (m_options.getItalic())        style |= static_cast<std::uint32_t>(sf::Text::Style::Italic);
    if (m_options.getUnderlined())    style |= static_cast<std::uint32_t>(sf::Text::Style::Underlined);
    if (m_options.getStrikeThrough()) style |= static_cast<std::uint32_t>(sf::Text::Style::StrikeThrough);
    m_text->setStyle(style);



    m_text->setFillColor(m_options.getColor());
}

bool Text::isLoaded() const { return m_loaded; }

void Text::setString(const std::string& content) {
    m_content = content;
    rebuildText();
}

void Text::update(sf::FloatRect& parentBounds, float dt) {
    (void)dt;
    if (!m_loaded) return;

    resize(parentBounds);

    float scale = m_uiloRef ? m_uiloRef->getScale() : 1.f;
    bool needRebuild = false;

    // Auto char size: 60% of the element's own height when setCharSize was not called.
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

void Text::render(sf::RenderTarget& target) {
    if (!m_loaded || !m_text) return;

    sf::Color cur = m_options.getColor();
    if (cur != m_lastColor) {
        m_lastColor = cur;
        m_text->setFillColor(cur);
    }

    sf::FloatRect lb = m_text->getLocalBounds();

    float x;
    if (m_options.getTextAlignX() == Align::CenterX)
        x = m_bounds.position.x + (m_bounds.size.x - lb.size.x) * 0.5f - lb.position.x;
    else if (m_options.getTextAlignX() == Align::Right)
        x = m_bounds.position.x + m_bounds.size.x - lb.size.x - lb.position.x;
    else
        x = m_bounds.position.x - lb.position.x;

    float y;
    if (m_options.getTextAlignY() == Align::CenterY)
        y = m_bounds.position.y + (m_bounds.size.y - lb.size.y) * 0.5f - lb.position.y;
    else if (m_options.getTextAlignY() == Align::Bottom)
        y = m_bounds.position.y + m_bounds.size.y - lb.size.y - lb.position.y;
    else
        y = m_bounds.position.y - lb.position.y;

    m_text->setPosition({std::round(x), std::round(y)});
    target.draw(*m_text);
    m_dirty = false;
}

}
