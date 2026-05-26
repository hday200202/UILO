#pragma once

#include <optional>

#include <SFML/Graphics.hpp>

#include "../Element.hpp"

namespace uilo {

class TextOptions {
public:
    TextOptions() = default;

    TextOptions& setFont(const std::string& path) { m_fontPath = path; m_fontRef = nullptr; return *this; }
    TextOptions& setFont(const sf::Font& font)    { m_fontRef = &font; m_fontPath.clear();  return *this; }
    TextOptions& setContent(const std::string& s) { m_content = s;          return *this; }
    TextOptions& setCharSize(unsigned int n)       { m_charSize = n;         return *this; }
    TextOptions& setColor(const sf::Color& c)      { m_color = c;            return *this; }
    TextOptions& setWrap(bool v)                   { m_wrap = v;             return *this; }
    TextOptions& setBold(bool v)                   { m_bold = v;             return *this; }
    TextOptions& setItalic(bool v)                 { m_italic = v;           return *this; }
    TextOptions& setUnderlined(bool v)             { m_underlined = v;       return *this; }
    TextOptions& setStrikeThrough(bool v)          { m_strikeThrough = v;    return *this; }
    TextOptions& setTextAlignX(Align a)            { m_textAlignX = a;       return *this; }
    TextOptions& setTextAlignY(Align a)            { m_textAlignY = a;       return *this; }

    const std::string& getFontPath()       const { return m_fontPath; }
    const sf::Font*    getFontRef()        const { return m_fontRef; }
    const std::string& getContent()        const { return m_content; }
    unsigned int       getCharSize()       const { return m_charSize.value_or(30); }
    bool               hasCharSize()       const { return m_charSize.has_value(); }
    sf::Color          getColor()          const { return m_color; }
    bool               getWrap()           const { return m_wrap; }
    bool               getBold()           const { return m_bold; }
    bool               getItalic()         const { return m_italic; }
    bool               getUnderlined()     const { return m_underlined; }
    bool               getStrikeThrough()  const { return m_strikeThrough; }
    Align              getTextAlignX()     const { return m_textAlignX; }
    Align              getTextAlignY()     const { return m_textAlignY; }

private:
    std::string     m_fontPath;
    const sf::Font* m_fontRef       = nullptr;
    std::string     m_content;
    std::optional<unsigned int> m_charSize;
    sf::Color       m_color         = sf::Color::White;
    bool            m_wrap          = false;
    bool            m_bold          = false;
    bool            m_italic        = false;
    bool            m_underlined    = false;
    bool            m_strikeThrough = false;
    Align           m_textAlignX    = Align::Left;
    Align           m_textAlignY    = Align::Top;
};

class Text : public Element {
public:
    explicit Text(Modifier modifier, TextOptions options = {}, const std::string& name = "");

    void update(sf::FloatRect& parentBounds, float dt) override;
    void render(sf::RenderTarget& target) override;

    void setString(const std::string& content);

    const TextOptions& getOptions() const      { return m_options; }
    void setOptions(const TextOptions& opts)   { m_options = opts; rebuildText(); }

    bool isLoaded() const;

private:
    std::string wrapContent(float maxWidth) const;
    void rebuildText();
    void init();

    TextOptions             m_options;
    sf::Font                m_ownedFont;
    const sf::Font*         m_fontPtr       = nullptr;
    std::string             m_content;
    unsigned int            m_charSize      = 30;  // resolved value (auto-computed or from options)
    float                   m_lastBoundsH   = 0.f;
    std::optional<sf::Text> m_text;
    sf::Color               m_lastColor     = sf::Color::White;
    float                   m_lastWrapWidth = 0.f;
    float                   m_lastScale     = 1.f;
    bool                    m_loaded        = false;
};

}