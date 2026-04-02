#pragma once

#include "../Element.hpp"

namespace uilo {

class Text : public Element {
public:
    Text(
        Modifier modifier,
        uint16_t fontSize,
        const std::string& string,
        const std::string& fontName,
        bool wordWrap = false,
        const std::string& name = ""
    );

    void setFontSize(uint16_t fontSize);
    uint16_t getFontSize() const;

    void setString(const std::string& string);
    const std::string& getString() const;

    void setFont(const std::string& fontName);
    const std::string& getFont() const;

    void setWordWrap(bool wordWrap);
    bool getWordWrap() const;

    void update(Bounds& parentBounds, float dt) override;
    void render(Renderer& renderer) override;

private:
    uint16_t m_fontSize = 12;
    std::string m_string = "Text";
    std::string m_fontName = "";
    bool m_wordWrap = false;
};

}