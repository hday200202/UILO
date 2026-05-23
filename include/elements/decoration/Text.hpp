#pragma once

#include <optional>
#include <cstdint>

#include <SFML/Graphics.hpp>

#include "../Element.hpp"

namespace uilo {

enum class TextOptions : uint16_t {
    NONE          = 0,
    LeftAlign     = 1 << 0,
    RightAlign    = 1 << 1,
    CenterX       = 1 << 2,
    CenterY       = 1 << 3,
    Wrap          = 1 << 4,
    Bold          = 1 << 5,
    Italic        = 1 << 6,
    Underlined    = 1 << 7,
    StrikeThrough = 1 << 8,
    TopAlign      = 1 << 9,
    BottomAlign   = 1 << 10,
};

inline TextOptions operator|(TextOptions a, TextOptions b) {
    return static_cast<TextOptions>(static_cast<uint16_t>(a) | static_cast<uint16_t>(b));
}

inline bool hasOption(TextOptions set, TextOptions flag) {
    return (static_cast<uint16_t>(set) & static_cast<uint16_t>(flag)) != 0;
}

class Text : public Element {
public:
    Text(Modifier modifier,
         const std::string& fontPath,
         const std::string& content,
         unsigned int charSize,
         TextOptions options = TextOptions::NONE,
         const std::string& name = "");

    Text(Modifier modifier,
         const sf::Font& font,
         const std::string& content,
         unsigned int charSize,
         TextOptions options = TextOptions::NONE,
         const std::string& name = "");

    void update(sf::FloatRect& parentBounds, float dt) override;
    void render(sf::RenderTarget& target) override;

    void setString(const std::string& content);
    void setOptions(TextOptions options) { m_options = options; rebuildText(); }

    bool isLoaded() const;

private:
    std::string wrapContent(float maxWidth) const;
    void rebuildText();
    void init();

    sf::Font              m_ownedFont;
    const sf::Font*       m_fontPtr = nullptr;

    std::string           m_content;
    unsigned int          m_charSize   = 30;
    std::optional<sf::Text> m_text;

    TextOptions           m_options         = TextOptions::NONE;
    sf::Color             m_lastColor       = sf::Color::White;
    float                 m_lastWrapWidth   = 0.f;
    bool                  m_loaded          = false;
};

}