#pragma once

#include <optional>
#include <string>

#include "../Element.hpp"
#include "../../renderer/Renderer.hpp"
#include "../../utils/Theme.hpp"

#include "../../utils/Themed.hpp"

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
    UILO_THEMED(TextOptions)

    /*
        inheritFrom(const TextOptions& prototype):
        - Params:   const TextOptions& prototype
        - Returns:  void
        - Desc:     Fills in every field the call site left alone from the
                    theme's prototype, and leaves the rest exactly as it was
                    set. Run when the element binds to its UILO, and again
                    whenever that UILO's theme changes.
    */
    void inheritFrom(const TextOptions& prototype) {
        m_color.inherit(prototype.m_color);
        m_wrap.inherit(prototype.m_wrap);
        m_bold.inherit(prototype.m_bold);
        m_italic.inherit(prototype.m_italic);
        m_underlined.inherit(prototype.m_underlined);
        m_strikeThrough.inherit(prototype.m_strikeThrough);
        m_textAlignX.inherit(prototype.m_textAlignX);
        m_textAlignY.inherit(prototype.m_textAlignY);
        m_outerPadding.inherit(prototype.m_outerPadding);
        m_innerPadding.inherit(prototype.m_innerPadding);
        m_charSize.inherit(prototype.m_charSize);
        m_colorRole.inherit(prototype.m_colorRole);
        m_fontPath.inherit(prototype.m_fontPath);
    }

    // Space kept outside this element, inside the slot its parent gave it. It
    // shrinks the element rather than displacing a sibling. Unset follows the theme's default for this type.
    TextOptions& setOuterPadding(float px)   { m_outerPadding.set(px); return *this; }
    TextOptions& clearOuterPadding()         { m_outerPadding.clear(); return *this; }
    float  getOuterPadding()     const { return m_outerPadding.get().value_or(0.f); }

    // Space kept between this element's edge and the box the text is laid out in. Unset follows the theme.
    TextOptions& setInnerPadding(float px)   { m_innerPadding.set(px); return *this; }
    TextOptions& clearInnerPadding()         { m_innerPadding.clear(); return *this; }
    float  getInnerPadding()     const { return m_innerPadding.get().value_or(0.f); }

    TextOptions& setFont(std::string_view path)   { m_fontPath.set(std::string(path)); return *this; }
    TextOptions& setContent(const std::string& s)   { m_content = s; return *this; }
    TextOptions& setCharSize(unsigned int n)        { m_charSize.set(n); return *this; }
    TextOptions& setColor(const Color& c)           { m_color.set(c); return *this; }
    TextOptions& setColorRole(const std::string& r) { m_colorRole.set(r); return *this; }
    TextOptions& setWrap(bool v)                    { m_wrap.set(v); return *this; }
    TextOptions& setBold(bool v)                    { m_bold.set(v); return *this; }
    TextOptions& setItalic(bool v)                  { m_italic.set(v); return *this; }
    TextOptions& setUnderlined(bool v)              { m_underlined.set(v); return *this; }
    TextOptions& setStrikeThrough(bool v)           { m_strikeThrough.set(v); return *this; }
    TextOptions& setTextAlignX(Align a)             { m_textAlignX.set(a); return *this; }
    TextOptions& setTextAlignY(Align a)             { m_textAlignY.set(a); return *this; }

    const std::string& getFontPath()      const;
    const std::string& getContent()       const { return m_content; }
    unsigned int       getCharSize()      const;
    bool               hasCharSize()      const { return m_charSize.get().has_value(); }
    Color              getColor()         const { return m_color.get(); }
    const std::string& getColorRole()     const { return m_colorRole.get(); }
    bool               getWrap()          const { return m_wrap.get(); }
    bool               getBold()          const { return m_bold.get(); }
    bool               getItalic()        const { return m_italic.get(); }
    bool               getUnderlined()    const { return m_underlined.get(); }
    bool               getStrikeThrough() const { return m_strikeThrough.get(); }
    Align              getTextAlignX()    const { return m_textAlignX.get(); }
    Align              getTextAlignY()    const { return m_textAlignY.get(); }

private:
    Themed<std::optional<float>> m_outerPadding;
    Themed<std::optional<float>> m_innerPadding;
    Themed<std::string> m_fontPath;
    std::string                 m_content;
    Themed<std::optional<unsigned int>> m_charSize;

    Themed<Color> m_color {Color::White};
    Themed<std::string> m_colorRole {"text"};

    Themed<bool>  m_wrap          {false};
    Themed<bool>  m_bold          {false};
    Themed<bool>  m_italic        {false};
    Themed<bool>  m_underlined    {false};
    Themed<bool>  m_strikeThrough {false};
    Themed<Align> m_textAlignX    {Align::Left};
    Themed<Align> m_textAlignY    {Align::Top};
    // The theme role this was constructed with; resolved at bind time.
    std::string m_themeRole;
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
    return m_fontPath.get();
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
    return m_charSize.get().value_or(30);
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
    /*
        applyTheme(const Theme& theme):
        - Params:   const Theme& theme
        - Returns:  void
        - Desc:     Inherits the unset options, then re-syncs the size the
                    element actually draws at. That cache is only refreshed by
                    update() while the options carry no size of their own, so a
                    size arriving from the theme has to be picked up here or it
                    would never be seen.
    */
    void applyTheme(const Theme& theme) override {
        m_options.inheritFrom(theme.cascade<TextOptions>(m_options.getThemeRole()));
        Element::applyTheme(theme);
        if (m_options.hasCharSize()) m_charSize = m_options.getCharSize();
        m_loaded = false;   /* the font may have come from the theme too */
        rebuildText();
    }

    float getOuterPadding() const override { return m_options.getOuterPadding(); }
    float getInnerPadding() const override { return m_options.getInnerPadding(); }

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
