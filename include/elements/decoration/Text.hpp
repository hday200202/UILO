#pragma once

#include <optional>
#include <string>

#include "../Element.hpp"
#include "../../renderer/Renderer.hpp"

namespace uilo {

class TextOptions {
public:
    TextOptions() = default;

    TextOptions& setFont(const std::string& path) { m_fontPath = path; return *this; }
    TextOptions& setContent(const std::string& s) { m_content = s;          return *this; }
    TextOptions& setCharSize(unsigned int n)       { m_charSize = n;         return *this; }
    TextOptions& setColor(const Color& c)      { m_color = c;            return *this; }
    TextOptions& setColorRole(const std::string& r){ m_colorRole = r;        return *this; }
    TextOptions& setWrap(bool v)                   { m_wrap = v;             return *this; }
    TextOptions& setBold(bool v)                   { m_bold = v;             return *this; }
    TextOptions& setItalic(bool v)                 { m_italic = v;           return *this; }
    TextOptions& setUnderlined(bool v)             { m_underlined = v;       return *this; }
    TextOptions& setStrikeThrough(bool v)          { m_strikeThrough = v;    return *this; }
    TextOptions& setTextAlignX(Align a)            { m_textAlignX = a;       return *this; }
    TextOptions& setTextAlignY(Align a)            { m_textAlignY = a;       return *this; }

    const std::string& getFontPath()       const { return m_fontPath; }
    const std::string& getContent()        const { return m_content; }
    unsigned int       getCharSize()       const { return m_charSize.value_or(30); }
    bool               hasCharSize()       const { return m_charSize.has_value(); }
    Color          getColor()          const { return m_color; }
    const std::string& getColorRole()  const { return m_colorRole; }
    bool               getWrap()           const { return m_wrap; }
    bool               getBold()           const { return m_bold; }
    bool               getItalic()         const { return m_italic; }
    bool               getUnderlined()     const { return m_underlined; }
    bool               getStrikeThrough()  const { return m_strikeThrough; }
    Align              getTextAlignX()     const { return m_textAlignX; }
    Align              getTextAlignY()     const { return m_textAlignY; }

private:
    std::string     m_fontPath;
    std::string     m_content;
    std::optional<unsigned int> m_charSize;
    Color       m_color         = Color::White;
    std::string m_colorRole;
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

    void update(Rectf& parentBounds, float dt) override;
    void render() override;

    void setString(const std::string& content);

    const TextOptions& getOptions() const      { return m_options; }
    TextOptions&       getOptions()            { return m_options; }
    void setOptions(const TextOptions& opts)   { m_options = opts; rebuildText(); }

    bool isLoaded() const;

private:
    std::string wrapContent(float maxWidth) const;
    void rebuildText();
    void init();

    TextOptions             m_options;
    uint32_t                m_fontId        = 0xFFFFFFFFu;
    std::string             m_content;
    std::string             m_wrappedContent;
    unsigned int            m_charSize      = 30;
    float                   m_lastBoundsH   = 0.f;
    Color                   m_lastColor     = Color::White;
    float                   m_lastWrapWidth = 0.f;
    float                   m_lastScale     = 1.f;
    bool                    m_loaded        = false;

    // Cached layout metrics for the current m_wrappedContent at the
    // current charSize/scale. Invalidated whenever any of those change
    // so render() can skip a full UTF-8 + glyph-table walk per frame.
    TextMetrics             m_cachedMetrics      = {};
    bool                    m_cachedMetricsValid = false;
};

}