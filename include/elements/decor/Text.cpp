#include "Text.hpp"

namespace uilo {

Text::Text(
    Modifier modifier, 
    uint16_t fontSize, 
    const std::string& string,
    const std::string& fontName, 
    bool wordWrap,
    const std::string& name
) : m_fontSize(fontSize), m_string(string), m_fontName(fontName), m_wordWrap(wordWrap) {
    m_modifier = modifier;
    m_type = ElementType::Text;
    m_name = name;
}

void Text::setFontSize(uint16_t fontSize) { m_fontSize = fontSize; }
uint16_t Text::getFontSize() const { return m_fontSize; }

void Text::setString(const std::string& string) { m_string = string; }
const std::string& Text::getString() const { return m_string; }

void Text::setFont(const std::string& fontName) { m_fontName = fontName; }
const std::string& Text::getFont() const { return m_fontName; }

void Text::setWordWrap(bool wordWrap) { m_wordWrap = wordWrap; }
bool Text::getWordWrap() const { return m_wordWrap; }

void Text::update(Bounds& parentBounds, float)  { resize(parentBounds); }
void Text::render(Renderer& renderer)           { renderer.drawText(*this); }

}