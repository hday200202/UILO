#pragma once

#include <functional>
#include <string>
#include <optional>
#include <limits>

#include "Interactible.hpp"
#include "../../utils/Theme.hpp"

namespace uilo {

/*
    TextboxOptions:
    - Desc:     Everything a Textbox draws and how it behaves: the font and text
                appearance, the box itself, the focus outline, the placeholder,
                the caret, the selection, the line-number gutter, and the
                editing rules. Colors come as a literal plus a role, where the
                role wins when it resolves against the active Palette and the
                literal is the fallback -- so setting a literal colour alone
                often does nothing until the role is cleared.
    - Multiline and wrap together decide the shape of the control. A single-line
      box is a field, a multiline one an editor, and a wrapping multiline one
      that was given a pixel height grows with its content.
*/
class TextboxOptions {
public:
    TextboxOptions() = default;

    // Space kept outside this element, inside the slot its parent gave it. It
    // shrinks the element rather than displacing a sibling. Unset follows
    // Theme::setOuterPadding().
    TextboxOptions& setOuterPadding(float px)   { m_outerPadding = px; return *this; }
    TextboxOptions& clearOuterPadding()         { m_outerPadding.reset(); return *this; }
    float  getOuterPadding()     const { return Theme::resolveOuterPadding(m_outerPadding, 0.f); }

    // Font
    TextboxOptions& setFont(std::string_view path) { m_fontPath = std::string(path); return *this; }

    // Text appearance
    TextboxOptions& setCharSize(unsigned int n)      { m_charSize = n;         return *this; }
    TextboxOptions& setTextColor(Color c)        { m_textColor = c;        return *this; }
    TextboxOptions& setTextColorRole(const std::string& r) { m_textColorRole = r; return *this; }
    TextboxOptions& setBold(bool v)                  { m_bold = v;             return *this; }
    TextboxOptions& setItalic(bool v)                { m_italic = v;           return *this; }
    TextboxOptions& setTextAlignX(Align a)           { m_textAlignX = a;       return *this; }
    TextboxOptions& setTextAlignY(Align a)           { m_textAlignY = a;       return *this; }

    // Box appearance
    TextboxOptions& setBackgroundColor(Color c)  { m_bgColor = c;          return *this; }
    TextboxOptions& setBackgroundColorRole(const std::string& r) { m_bgColorRole = r; return *this; }
    TextboxOptions& setRounding(float r)             { m_rounding = r;         return *this; }
    // Uniform padding: setInnerPadding is the spelling every other Options
    // uses, setPadding is kept as its alias, and the per-side setters below
    // override either one.
    TextboxOptions& setInnerPadding(float p)         { m_paddingLeft = m_paddingRight = m_paddingTop = m_paddingBottom = p; return *this; }
    TextboxOptions& setPadding(float p)              { m_paddingLeft = m_paddingRight = m_paddingTop = m_paddingBottom = p; return *this; }
    TextboxOptions& setPaddingLeft(float p)          { m_paddingLeft = p;       return *this; }
    TextboxOptions& setPaddingRight(float p)         { m_paddingRight = p;      return *this; }
    TextboxOptions& setPaddingTop(float p)           { m_paddingTop = p;        return *this; }
    TextboxOptions& setPaddingBottom(float p)        { m_paddingBottom = p;     return *this; }

    // Focus outline, drawn only while the textbox is active. Thickness falls
    // back to Theme::setOutlineThickness() and an unnamed role to
    // Theme::setOutlineColorRole(), as elsewhere.
    TextboxOptions& setOutlineColor(Color c)     { m_outlineColor = c;     return *this; }
    TextboxOptions& setOutlineColorRole(const std::string& r) { m_outlineColorRole = r; return *this; }
    TextboxOptions& setOutlineThickness(float t)     { m_outlineThickness = t; return *this; }

    // Placeholder
    TextboxOptions& setPlaceholder(const std::string& s) { m_placeholder = s;  return *this; }
    TextboxOptions& setPlaceholderColor(Color c) { m_placeholderColor = c; return *this; }
    TextboxOptions& setPlaceholderColorRole(const std::string& r) { m_placeholderColorRole = r; return *this; }

    // Cursor
    TextboxOptions& setCursorColor(Color c)      { m_cursorColor = c;      return *this; }
    TextboxOptions& setCursorColorRole(const std::string& r) { m_cursorColorRole = r; return *this; }
    TextboxOptions& setCursorWidth(float w)          { m_cursorWidth = w;      return *this; }
    TextboxOptions& setCursorBlinkRate(float s)      { m_blinkRate = s;        return *this; }

    // Selection
    TextboxOptions& setSelectionColor(Color c)   { m_selectionColor = c;   return *this; }
    TextboxOptions& setSelectionColorRole(const std::string& r) { m_selectionColorRole = r; return *this; }

    // Line numbers: a gutter down the left edge, numbering the logical lines.
    // Multiline only -- a single-line box has nothing to number. The gutter takes
    // its width off the text area, so the cursor, hit testing and wrapping all
    // follow it without any further configuration.
    TextboxOptions& setShowLineNumbers(bool v)          { m_showLineNumbers = v; return *this; }
    // Width is normally measured from the widest number actually present; this
    // sets a floor in digits, so a short file does not have a narrower gutter
    // than a long one.
    TextboxOptions& setLineNumberMinDigits(int n)        { m_lineNumberMinDigits = n; return *this; }
    TextboxOptions& setLineNumberPadding(float p)        { m_lineNumberPadding = p; return *this; }
    // Where each number sits within the gutter. Right (the default) lines the
    // digits up against the text, which is how an editor normally reads.
    TextboxOptions& setLineNumberAlign(Align a)          { m_lineNumberAlign = a; return *this; }
    TextboxOptions& setLineNumberColor(Color c)          { m_lineNumberColor = c; return *this; }
    TextboxOptions& setLineNumberColorRole(const std::string& r) { m_lineNumberColorRole = r; return *this; }
    // The line the cursor is on, so it stands out from the rest. Unset (alpha 0)
    // draws every number in the base colour.
    TextboxOptions& setCurrentLineNumberColor(Color c)   { m_currentLineNumberColor = c; return *this; }
    TextboxOptions& setCurrentLineNumberColorRole(const std::string& r) { m_currentLineNumberColorRole = r; return *this; }
    // Gutter background. Transparent by default, so it sits on the box's own fill.
    TextboxOptions& setLineNumberBackgroundColor(Color c) { m_lineNumberBgColor = c; return *this; }
    TextboxOptions& setLineNumberBackgroundColorRole(const std::string& r) { m_lineNumberBgColorRole = r; return *this; }
    // Styling for the numbers themselves, independent of the editor text.
    TextboxOptions& setLineNumberBold(bool v)         { m_lineNumberBold = v; return *this; }
    TextboxOptions& setLineNumberItalic(bool v)       { m_lineNumberItalic = v; return *this; }
    // The cursor's own line can be styled differently again -- bold-only-current
    // is a common editor look. Unset, it follows the two above.
    TextboxOptions& setCurrentLineNumberBold(bool v)   { m_currentLineNumberBold = v; return *this; }
    TextboxOptions& setCurrentLineNumberItalic(bool v) { m_currentLineNumberItalic = v; return *this; }
    // Unset (the default) draws the numbers at the editor's own char size.
    TextboxOptions& setLineNumberCharSize(unsigned int n) { m_lineNumberCharSize = n; return *this; }

    // Behaviour
    TextboxOptions& setMultiline(bool v)             { m_multiline = v;        return *this; }
    TextboxOptions& setWrap(bool v)                  { m_wrap = v;             return *this; }
    TextboxOptions& setMaxResizeLines(int n)         { m_maxResizeLines = n;   return *this; }
    TextboxOptions& setMaxLength(int n)              { m_maxLength = n;        return *this; }
    TextboxOptions& setPasswordMode(bool v)          { m_passwordMode = v;     return *this; }
    // Pixels travelled per unit of scroll delta, matching ColumnOptions so a
    // textbox and a scrollable column feel the same under one gesture.
    TextboxOptions& setScrollSpeed(float s)          { m_scrollSpeed = s;      return *this; }
    // How many spaces the Tab key inserts. Tab always inserts spaces rather than
    // a literal '\t': the renderer has no tab-advance logic, so a real tab
    // character would draw as a missing-glyph box.
    TextboxOptions& setTabWidth(int n)               { m_tabWidth = n;         return *this; }

    // Callbacks
    TextboxOptions& setOnStringChanged(std::function<void(const std::string&)> f) {
        m_onStringChanged = std::move(f); return *this;
    }
    TextboxOptions& setOnEnterPressed(std::function<void(const std::string&)> f) {
        m_onEnterPressed = std::move(f); return *this;
    }

    const std::string& getFontPath()         const;
    unsigned int       getCharSize()         const;
    bool               hasCharSize()         const { return m_charSize.has_value(); }
    Color              getTextColor()        const { return m_textColor; }
    const std::string& getTextColorRole()    const { return m_textColorRole; }
    bool               getBold()             const { return m_bold; }
    bool               getItalic()           const { return m_italic; }
    Align              getTextAlignX()       const { return m_textAlignX; }
    Align              getTextAlignY()       const { return m_textAlignY; }
    Color              getBackgroundColor()  const { return m_bgColor; }
    const std::string& getBackgroundColorRole() const { return m_bgColorRole; }
    float              getRounding()         const;
    float              getPaddingLeft()      const { return m_paddingLeft; }
    float              getPaddingRight()     const { return m_paddingRight; }
    float              getPaddingTop()       const { return m_paddingTop; }
    float              getPaddingBottom()    const { return m_paddingBottom; }
    Color              getOutlineColor()     const { return m_outlineColor; }
    const std::string& getOutlineColorRole() const { return m_outlineColorRole; }
    float              getOutlineThickness() const { return m_outlineThickness; }
    const std::string& getPlaceholder()      const { return m_placeholder; }
    Color              getPlaceholderColor() const { return m_placeholderColor; }
    const std::string& getPlaceholderColorRole() const { return m_placeholderColorRole; }
    Color              getCursorColor()      const { return m_cursorColor; }
    const std::string& getCursorColorRole()  const { return m_cursorColorRole; }
    float              getCursorWidth()      const { return m_cursorWidth; }
    float              getBlinkRate()        const { return m_blinkRate; }
    Color              getSelectionColor()   const { return m_selectionColor; }
    const std::string& getSelectionColorRole() const { return m_selectionColorRole; }
    bool               getShowLineNumbers()  const { return m_showLineNumbers; }
    int                getLineNumberMinDigits() const { return m_lineNumberMinDigits; }
    float              getLineNumberPadding()   const { return m_lineNumberPadding; }
    Align              getLineNumberAlign()     const { return m_lineNumberAlign; }
    Color              getLineNumberColor()     const { return m_lineNumberColor; }
    const std::string& getLineNumberColorRole() const { return m_lineNumberColorRole; }
    Color              getCurrentLineNumberColor()     const { return m_currentLineNumberColor; }
    const std::string& getCurrentLineNumberColorRole() const { return m_currentLineNumberColorRole; }
    Color              getLineNumberBackgroundColor()     const { return m_lineNumberBgColor; }
    const std::string& getLineNumberBackgroundColorRole() const { return m_lineNumberBgColorRole; }
    bool               getLineNumberBold()          const { return m_lineNumberBold; }
    bool               getLineNumberItalic()        const { return m_lineNumberItalic; }
    bool               getCurrentLineNumberBold()   const { return m_currentLineNumberBold.value_or(m_lineNumberBold); }
    bool               getCurrentLineNumberItalic() const { return m_currentLineNumberItalic.value_or(m_lineNumberItalic); }
    unsigned int       getLineNumberCharSize()      const { return m_lineNumberCharSize.value_or(0u); }
    bool               hasLineNumberCharSize()      const { return m_lineNumberCharSize.has_value(); }

    bool               getMultiline()        const { return m_multiline; }
    bool               getWrap()             const { return m_wrap; }
    int                getMaxResizeLines()   const { return m_maxResizeLines; }
    int                getMaxLength()        const { return m_maxLength; }
    bool               getPasswordMode()     const { return m_passwordMode; }
    int                getTabWidth()         const { return m_tabWidth; }
    float              getScrollSpeed()      const { return m_scrollSpeed; }
    const std::function<void(const std::string&)>& getOnStringChanged() const { return m_onStringChanged; }
    const std::function<void(const std::string&)>& getOnEnterPressed()  const { return m_onEnterPressed; }

private:
    std::optional<float> m_outerPadding;
    std::string        m_fontPath;
    std::optional<unsigned int> m_charSize;
    Color              m_textColor        = Color::White;
    std::string        m_textColorRole    = "text";
    bool               m_bold             = false;
    bool               m_italic           = false;
    Align              m_textAlignX       = Align::Left;
    Align              m_textAlignY       = Align::CenterY;
    Color              m_bgColor          = Color{50, 50, 50};
    std::string        m_bgColorRole      = "panelAlt";
    std::optional<float>              m_rounding;
    float              m_paddingLeft      = 6.f;
    float              m_paddingRight     = 6.f;
    float              m_paddingTop       = 6.f;
    float              m_paddingBottom    = 6.f;
    Color              m_outlineColor     = Color::White;
    std::string        m_outlineColorRole = "accent";
    float m_outlineThickness = 0.f;
    std::string        m_placeholder;
    Color              m_placeholderColor  = Color{128, 128, 128};
    std::string        m_placeholderColorRole = "textDim";
    Color              m_cursorColor       = Color::White;
    std::string        m_cursorColorRole  = "text";
    float              m_cursorWidth       = 2.f;
    float              m_blinkRate         = 1.0f;   /* full blink cycle in seconds */
    Color              m_selectionColor    = Color{70, 130, 200, 160};
    std::string        m_selectionColorRole;
    bool        m_showLineNumbers      = false;
    int         m_lineNumberMinDigits  = 2;
    float       m_lineNumberPadding    = 6.f;
    Align       m_lineNumberAlign      = Align::Right;
    Color       m_lineNumberColor      = Color{128, 128, 128};
    std::string m_lineNumberColorRole  = "textDim";
    Color       m_currentLineNumberColor     = Color{0, 0, 0, 0};   /* a == 0 -> use base */
    std::string m_currentLineNumberColorRole = "text";
    Color       m_lineNumberBgColor     = Color{0, 0, 0, 0};
    std::string m_lineNumberBgColorRole;
    bool                        m_lineNumberBold   = false;
    bool                        m_lineNumberItalic = false;
    std::optional<bool>         m_currentLineNumberBold;
    std::optional<bool>         m_currentLineNumberItalic;
    std::optional<unsigned int> m_lineNumberCharSize;

    bool               m_multiline         = false;
    bool               m_wrap              = true;   /* wrap text when multiline */
    int                m_maxResizeLines    = 0;   /* 0 = unlimited; >0 = clamp height, scroll beyond */
    int                m_maxLength         = 0;   /* 0 = unlimited */
    bool               m_passwordMode      = false;
    int                m_tabWidth          = 4;
    float              m_scrollSpeed       = 40.f;   // same default as a Column
    std::function<void(const std::string&)> m_onStringChanged;
    std::function<void(const std::string&)> m_onEnterPressed;
};

// ---------------------------------------------------------------------------

/*
    getFontPath():
    - Params:   none
    - Returns:  const std::string&
    - Desc:     Font for the text, falling back to the active Theme's when this
                box was not given one. May be a path or a name the Resources
                font registry knows.
*/
inline const std::string& TextboxOptions::getFontPath() const {
    return Theme::resolveFont(m_fontPath);
}


/*
    getCharSize():
    - Params:   none
    - Returns:  unsigned int
    - Desc:     Character size in unscaled pixels, resolved in three steps: the
                value this box was given, then the active Theme's, then 18. Ask
                hasCharSize() to tell a real setting from the fallback, which is
                what the box does before deciding to size the text from its own
                height instead.
*/
inline unsigned int TextboxOptions::getCharSize() const {
    return Theme::resolveCharSize(m_charSize, 18);
}


/*
    getRounding():
    - Params:   none
    - Returns:  float
    - Desc:     Corner radius, resolved in three steps: the value this box was
                given, then the active Theme's, then 0.
*/
inline float TextboxOptions::getRounding() const {
    return Theme::resolveRounding(m_rounding, 0.f);
}


/*
    Textbox:
    - Desc:     An editable text field or multiline editor. The text is held as
                UTF-32 so an index is a codepoint rather than a byte, which is
                what keeps cursor movement, selection and word jumps simple;
                UTF-8 is produced only at the renderer and API boundaries.
    - Multiline plus wrap builds a soft-wrapped display string alongside the
      real one, with a table of where breaks were inserted so display and text
      indices map back and forth. Everything positional -- the caret, hit
      testing, selection rectangles -- goes through that mapping.
    - Height works one of two ways. A pixel height is a starting size the box
      may grow past as content is added, publishing the new height back to its
      parent; a percent height means the parent owns the slot, so the box fills
      it and taller content scrolls. Growing a percent box would mean rewriting
      its declared height to pixels, which severs it from the layout and stops
      it following a window resize.
    - Layout metrics are cached and rebuilt only when the text, size, scale or
      wrap width changes, so an idle frame costs no measurement.
*/
class Textbox : public Interactible {
public:
    float getOuterPadding() const override { return m_options.getOuterPadding(); }

    explicit Textbox(
        Modifier modifier,
        TextboxOptions options = {},
        const std::string& name = ""
    );

    void update(Rectf& parentBounds, float dt) override;
    void render() override;
    bool checkLeftClick(const Vec2f& mousePos) override;
    bool checkHover(const Vec2f& mousePos) override;
    bool checkScroll(
        const Vec2f& mousePos,
        float delta,
        bool precise = false,
        bool momentum = false
    ) override;
    void onDeactivate() override;

    // Routed here by UILO while this is the active interactible.
    void handleTextInput(char32_t unicode) override;
    void handleKeyInput(SDL_Keycode key, bool shift, bool ctrl, bool gui) override;
    bool wantsTextInput() const override { return true; }

    // A multiline box scrolls a view, so a flick over one should coast the way
    // it does over a scrollable column. A single-line box has nothing to coast.
    bool wantsScrollMomentum() const override { return m_options.getMultiline(); }

    std::string getString() const;
    void        setString(const std::string& s);
    bool        isFocused() const { return m_focused; }

    const TextboxOptions& getOptions() const { return m_options; }
    TextboxOptions&       getOptions()       { return m_options; }

private:
    bool          autoGrows()             const;
    float         maxScrollY(int lineCount, float lh) const;
    std::string   resolvedFontPath() const;
    Rectf         textArea()              const;
    Rectf         gutterArea()            const;
    float         gutterWidth()           const;
    void          recomputeGutterWidth(class Renderer& renderer, float pxH);
    void          renderLineNumbers(class Renderer& renderer, float pxH);
    size_t        logicalLineOfCursor()   const;
    void          insertTab();
    float         lineHeight()            const;
    Vec2f         charScreenPos(size_t i) const;
    size_t        hitTestChar(Vec2f screenPos) const;
    void          ensureCursorVisible();
    bool          hasSelection()          const;
    void          deleteSelection();
    void          resetBlink();
    void          rebuildSfText();
    void          rebuildWrapped();
    void          computeTextOrigin();
    std::u32string displayText()          const;

    // Soft-wrap index mapping, in both directions.
    size_t        textToDisplay(size_t textIdx) const;
    size_t        displayToText(size_t dispIdx) const;

    size_t lineStart(size_t pos)  const;
    size_t lineEnd(size_t pos)    const;
    size_t wordLeft(size_t pos)   const;
    size_t wordRight(size_t pos)  const;

    TextboxOptions          m_options;
    std::u32string          m_text;
    size_t                  m_cursorPos     = 0;
    size_t                  m_anchorPos     = 0;   /* same as cursor = no selection */
    bool                    m_focused       = false;
    float                   m_blinkTimer    = 0.f;
    bool                    m_cursorVisible = true;
    float                   m_scrollOffsetX = 0.f;
    float                   m_scrollOffsetY = 0.f;
    float                   m_preferredX    = 0.f;   /* preserved column x for up/down nav */

    // Soft-wrap state (multiline+wrap mode)
    std::string             m_wrappedDisplay;   /* UTF-8 display string with soft newlines */
    std::u32string          m_displayU32;   /* UTF-32 view of m_wrappedDisplay */
    std::vector<Vec2f>      m_charPositions;   /* per-codepoint position (size = m_displayU32.size()+1) */
    std::vector<size_t>     m_softWrapAt;   /* text indices where soft wrap was inserted */
    float                   m_lastWrapWidth = 0.f;
    float                   m_lineHeightCache   = 0.f;
    float                   m_initialHeight     = 0.f;
    bool                    m_initialHeightSet  = false;
    bool                    m_mouseDown         = false;
    bool                    m_dragging          = false;   /* true once mouse has moved past threshold */
    bool                    m_needsCursorScroll = false;   /* scroll to cursor only when it moved */
    unsigned int            m_autoCharSize      = 0;   /* resolved auto char size (0 = not yet computed) */
    Vec2f                   m_mouseDownPos      = {0.f, 0.f};
    // TODO: font handle (FreeType) — deferred
    Rectf                   m_stableLineBounds;
    // Cached so textArea() stays const and cheap: the width depends on the font
    // and the line count, neither of which is available where textArea is called.
    float                   m_gutterWidth   = 0.f;
    float                   m_lastScale     = 0.f;
    bool                    m_textDirty     = true;
    Vec2f                   m_textOrigin;
};

} // namespace uilo