#include "Text.hpp"
#include "../../utils/Alignment.hpp"

#include <sstream>

namespace uilo {

Text::Text(
    Modifier modifier,
    TextOptions options,
    const std::string& name
) : m_options(options), m_content(options.getContent()), m_charSize(options.getCharSize()) {
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
    m_lastColor = m_modifier.getColor();
    rebuildText();
    m_loaded = true;
}

std::string Text::wrapContent(float maxWidth) const {
    if (maxWidth <= 0.f) return m_content;

    sf::Text probe(*m_fontPtr, "", m_charSize);
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

    float wrapWidth = m_bounds.size.x;
    std::string str = (m_options.getWrap() && wrapWidth > 0.f)
                    ? wrapContent(wrapWidth)
                    : m_content;

    m_text.emplace(*m_fontPtr, str, m_charSize);

    std::uint32_t style = 0;
    if (m_options.getBold())          style |= static_cast<std::uint32_t>(sf::Text::Style::Bold);
    if (m_options.getItalic())        style |= static_cast<std::uint32_t>(sf::Text::Style::Italic);
    if (m_options.getUnderlined())    style |= static_cast<std::uint32_t>(sf::Text::Style::Underlined);
    if (m_options.getStrikeThrough()) style |= static_cast<std::uint32_t>(sf::Text::Style::StrikeThrough);
    m_text->setStyle(style);

    if (m_options.getTextAlignX() == Align::CenterX)
        m_text->setLineAlignment(sf::Text::LineAlignment::Center);
    else if (m_options.getTextAlignX() == Align::Right)
        m_text->setLineAlignment(sf::Text::LineAlignment::Right);
    else
        m_text->setLineAlignment(sf::Text::LineAlignment::Left);

    m_text->setFillColor(m_modifier.getColor());
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

    if (m_options.getWrap() && m_bounds.size.x != m_lastWrapWidth) {
        m_lastWrapWidth = m_bounds.size.x;
        rebuildText();
    }
}

void Text::render(sf::RenderTarget& target) {
    if (!m_loaded || !m_text) return;

    sf::Color cur = m_modifier.getColor();
    if (cur != m_lastColor) {
        m_lastColor = cur;
        m_text->setFillColor(cur);
    }

    sf::FloatRect lb = m_text->getLocalBounds();

    float x;
    if (m_options.getTextAlignX() == Align::CenterX)
        x = m_bounds.position.x + m_bounds.size.x * 0.5f;
    else if (m_options.getTextAlignX() == Align::Right)
        x = m_bounds.position.x + m_bounds.size.x;
    else
        x = m_bounds.position.x - lb.position.x;

    float y;
    if (m_options.getTextAlignY() == Align::CenterY)
        y = m_bounds.position.y + (m_bounds.size.y - lb.size.y) * 0.5f - lb.position.y;
    else if (m_options.getTextAlignY() == Align::Bottom)
        y = m_bounds.position.y + m_bounds.size.y - lb.size.y - lb.position.y;
    else
        y = m_bounds.position.y - lb.position.y;

    m_text->setPosition({x, y});
    target.draw(*m_text);
}

}
