#pragma once

#include <optional>
#include <string>

#include "../Element.hpp"
#include "../../renderer/Renderer.hpp"
#include "../../utils/Theme.hpp"

namespace uilo {

/*
    TextOptions:
    - Desc:     Everything a Text draws: the font, the string, the character
                size, the colour, the style flags, and where the glyphs sit
                inside the element's bounds. The colour comes as a literal plus
                a role, where the role wins when it resolves against the active
                Palette and the literal is the fallback. Character size is left
                unset by default, which makes the element size its own text from
                its height.
    - setTextAlignX / setTextAlignY place the glyphs within the element, which
      is separate from the Modifier's align, which places the element within its
      parent.
*/
class TextOptions {
public:
    TextOptions() = default;

    TextOptions& setFont(std::string_view path)   { m_fontPath = std::string(path); return *this; }
    TextOptions& setContent(const std::string& s)   { m_content = s; return *this; }
    TextOptions& setCharSize(unsigned int n)        { m_charSize = n; return *this; }
    TextOptions& setColor(const Color& c)           { m_color = c; return *this; }
    TextOptions& setColorRole(const std::string& r) { m_colorRole = r; return *this; }
    TextOptions& setWrap(bool v)                    { m_wrap = v; return *this; }
    TextOptions& setBold(bool v)                    { m_bold = v; return *this; }
    TextOptions& setItalic(bool v)                  { m_italic = v; return *this; }
    TextOptions& setUnderlined(bool v)              { m_underlined = v; return *this; }
    TextOptions& setStrikeThrough(bool v)           { m_strikeThrough = v; return *this; }
    TextOptions& setTextAlignX(Align a)             { m_textAlignX = a; return *this; }
    TextOptions& setTextAlignY(Align a)             { m_textAlignY = a; return *this; }

    const std::string& getFontPath()      const;
    const std::string& getContent()       const { return m_content; }
    unsigned int       getCharSize()      const;
    bool               hasCharSize()      const { return m_charSize.has_value(); }
    Color              getColor()         const { return m_color; }
    const std::string& getColorRole()     const { return m_colorRole; }
    bool               getWrap()          const { return m_wrap; }
    bool               getBold()          const { return m_bold; }
    bool               getItalic()        const { return m_italic; }
    bool               getUnderlined()    const { return m_underlined; }
    bool               getStrikeThrough() const { return m_strikeThrough; }
    Align              getTextAlignX()    const { return m_textAlignX; }
    Align              getTextAlignY()    const { return m_textAlignY; }

private:
    std::string                 m_fontPath;
    std::string                 m_content;
    std::optional<unsigned int> m_charSize;

    Color       m_color     = Color::White;
    std::string m_colorRole = "text";

    bool  m_wrap          = false;
    bool  m_bold          = false;
    bool  m_italic        = false;
    bool  m_underlined    = false;
    bool  m_strikeThrough = false;
    Align m_textAlignX    = Align::Left;
    Align m_textAlignY    = Align::Top;
};


/*
    getFontPath():
    - Params:   none
    - Returns:  const std::string&
    - Desc:     Font for the string, falling back to the active Theme's when
                this element was not given one. May be either a path or a name
                the Resources font registry knows; Text resolves which at load
                time.
*/
inline const std::string& TextOptions::getFontPath() const {
    return Theme::resolveFont(m_fontPath);
}


/*
    getCharSize():
    - Params:   none
    - Returns:  unsigned int
    - Desc:     Character size in unscaled pixels, resolved in three steps: the
                value this element was given, then the active Theme's, then 30.
                Ask hasCharSize() to tell a real setting from the fallback,
                which is what Text does before deciding to size the glyphs from
                its own height instead.
*/
inline unsigned int TextOptions::getCharSize() const {
    return Theme::resolveCharSize(m_charSize, 30);
}


/*
    Text:
    - Desc:     A string drawn with a loaded font. The font is loaded on the
                first update, once the element is bound to a UILO and so has a
                renderer to load it with, which is why a Text built before the
                page is added still works. With no explicit char size the glyphs
                are sized from the element's height, so a Text in a fixed-height
                row scales with it. Optional word wrapping re-flows the string
                whenever the element's width changes. Layout metrics are cached
                and only re-measured when the string, size, scale or wrap width
                changes, so drawing does not walk the UTF-8 and the glyph table
                every frame.
    - setString() writes to a live string separate from the options, so a
      runtime update is read through getString() rather than
      getOptions().getContent().
*/
class Text : public Element {
public:
    explicit Text(
        Modifier modifier,
        TextOptions options = {},
        const std::string& name = ""
    );

    void update(Rectf& parentBounds, float dt) override;
    void render() override;

    void               setString(const std::string& content);
    const std::string& getString() const { return m_content; }

    const TextOptions& getOptions() const { return m_options; }
    TextOptions&       getOptions()       { return m_options; }
    void               setOptions(const TextOptions& opts) { m_options = opts; rebuildText(); }

    bool isLoaded() const;

private:
    std::string wrapContent(float maxWidth) const;
    void        rebuildText();
    void        init();

    TextOptions  m_options;
    uint32_t     m_fontId = 0xFFFFFFFFu;
    std::string  m_content;
    std::string  m_wrappedContent;
    unsigned int m_charSize      = 30;
    Color        m_lastColor     = Color::White;
    float        m_lastWrapWidth = 0.f;
    float        m_lastScale     = 1.f;
    bool         m_loaded        = false;

    // Layout metrics for the current wrapped string at the current char size and
    // scale, invalidated whenever any of those change.
    TextMetrics m_cachedMetrics      = {};
    bool        m_cachedMetricsValid = false;
};

}
