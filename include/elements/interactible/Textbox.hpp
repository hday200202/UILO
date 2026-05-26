#pragma once

#include <functional>
#include <string>
#include <optional>
#include <limits>

#include "Interactible.hpp"

namespace uilo {

class TextboxOptions {
public:
    TextboxOptions() = default;

    // Font
    TextboxOptions& setFont(const std::string& path) { m_fontPath = path; return *this; }

    // Text appearance
    TextboxOptions& setCharSize(unsigned int n)      { m_charSize = n;         return *this; }
    TextboxOptions& setTextColor(Color c)        { m_textColor = c;        return *this; }
    TextboxOptions& setBold(bool v)                  { m_bold = v;             return *this; }
    TextboxOptions& setItalic(bool v)                { m_italic = v;           return *this; }
    TextboxOptions& setTextAlignX(Align a)           { m_textAlignX = a;       return *this; }
    TextboxOptions& setTextAlignY(Align a)           { m_textAlignY = a;       return *this; }

    // Box appearance
    TextboxOptions& setBackgroundColor(Color c)  { m_bgColor = c;          return *this; }
    TextboxOptions& setRounding(float r)             { m_rounding = r;         return *this; }
    // Uniform padding (sets all four sides)
    TextboxOptions& setPadding(float p)              { m_paddingLeft = m_paddingRight = m_paddingTop = m_paddingBottom = p; return *this; }
    TextboxOptions& setPaddingLeft(float p)          { m_paddingLeft = p;       return *this; }
    TextboxOptions& setPaddingRight(float p)         { m_paddingRight = p;      return *this; }
    TextboxOptions& setPaddingTop(float p)           { m_paddingTop = p;        return *this; }
    TextboxOptions& setPaddingBottom(float p)        { m_paddingBottom = p;     return *this; }

    // Focus outline (drawn when textbox is active)
    TextboxOptions& setOutlineColor(Color c)     { m_outlineColor = c;     return *this; }
    TextboxOptions& setOutlineThickness(float t)     { m_outlineThickness = t; return *this; }

    // Placeholder
    TextboxOptions& setPlaceholder(const std::string& s) { m_placeholder = s;  return *this; }
    TextboxOptions& setPlaceholderColor(Color c) { m_placeholderColor = c; return *this; }

    // Cursor
    TextboxOptions& setCursorColor(Color c)      { m_cursorColor = c;      return *this; }
    TextboxOptions& setCursorWidth(float w)          { m_cursorWidth = w;      return *this; }
    TextboxOptions& setCursorBlinkRate(float s)      { m_blinkRate = s;        return *this; }

    // Selection
    TextboxOptions& setSelectionColor(Color c)   { m_selectionColor = c;   return *this; }

    // Behaviour
    TextboxOptions& setMultiline(bool v)             { m_multiline = v;        return *this; }
    TextboxOptions& setWrap(bool v)                  { m_wrap = v;             return *this; }
    TextboxOptions& setMaxResizeLines(int n)         { m_maxResizeLines = n;   return *this; }
    TextboxOptions& setMaxLength(int n)              { m_maxLength = n;        return *this; }
    TextboxOptions& setPasswordMode(bool v)          { m_passwordMode = v;     return *this; }

    // Callbacks
    TextboxOptions& setOnStringChanged(std::function<void(const std::string&)> f) {
        m_onStringChanged = std::move(f); return *this;
    }
    TextboxOptions& setOnEnterPressed(std::function<void(const std::string&)> f) {
        m_onEnterPressed = std::move(f); return *this;
    }

    const std::string& getFontPath()         const { return m_fontPath; }
    unsigned int       getCharSize()         const { return m_charSize.value_or(18); }
    bool               hasCharSize()         const { return m_charSize.has_value(); }
    Color              getTextColor()        const { return m_textColor; }
    bool               getBold()             const { return m_bold; }
    bool               getItalic()           const { return m_italic; }
    Align              getTextAlignX()       const { return m_textAlignX; }
    Align              getTextAlignY()       const { return m_textAlignY; }
    Color              getBackgroundColor()  const { return m_bgColor; }
    float              getRounding()         const { return m_rounding; }
    float              getPaddingLeft()      const { return m_paddingLeft; }
    float              getPaddingRight()     const { return m_paddingRight; }
    float              getPaddingTop()       const { return m_paddingTop; }
    float              getPaddingBottom()    const { return m_paddingBottom; }
    Color              getOutlineColor()     const { return m_outlineColor; }
    float              getOutlineThickness() const { return m_outlineThickness; }
    const std::string& getPlaceholder()      const { return m_placeholder; }
    Color              getPlaceholderColor() const { return m_placeholderColor; }
    Color              getCursorColor()      const { return m_cursorColor; }
    float              getCursorWidth()      const { return m_cursorWidth; }
    float              getBlinkRate()        const { return m_blinkRate; }
    Color              getSelectionColor()   const { return m_selectionColor; }
    bool               getMultiline()        const { return m_multiline; }
    bool               getWrap()             const { return m_wrap; }
    int                getMaxResizeLines()   const { return m_maxResizeLines; }
    int                getMaxLength()        const { return m_maxLength; }
    bool               getPasswordMode()     const { return m_passwordMode; }
    const std::function<void(const std::string&)>& getOnStringChanged() const { return m_onStringChanged; }
    const std::function<void(const std::string&)>& getOnEnterPressed()  const { return m_onEnterPressed; }

private:
    std::string        m_fontPath;
    std::optional<unsigned int> m_charSize;
    Color              m_textColor        = Color::White;
    bool               m_bold             = false;
    bool               m_italic           = false;
    Align              m_textAlignX       = Align::Left;
    Align              m_textAlignY       = Align::CenterY;
    Color              m_bgColor          = Color{50, 50, 50};
    float              m_rounding         = 0.f;
    float              m_paddingLeft      = 6.f;
    float              m_paddingRight     = 6.f;
    float              m_paddingTop       = 6.f;
    float              m_paddingBottom    = 6.f;
    Color              m_outlineColor     = Color::White;
    float              m_outlineThickness = 0.f;
    std::string        m_placeholder;
    Color              m_placeholderColor  = Color{128, 128, 128};
    Color              m_cursorColor       = Color::White;
    float              m_cursorWidth       = 2.f;
    float              m_blinkRate         = 1.0f;   // full blink cycle in seconds
    Color              m_selectionColor    = Color{70, 130, 200, 160};
    bool               m_multiline         = false;
    bool               m_wrap              = true;    // wrap text when multiline
    int                m_maxResizeLines    = 0;       // 0 = unlimited; >0 = clamp height, scroll beyond
    int                m_maxLength         = 0;       // 0 = unlimited
    bool               m_passwordMode      = false;
    std::function<void(const std::string&)> m_onStringChanged;
    std::function<void(const std::string&)> m_onEnterPressed;
};

// ---------------------------------------------------------------------------

class Textbox : public Interactible {
public:
    explicit Textbox(Modifier modifier, TextboxOptions options = {}, const std::string& name = "");

    void update(Rectf& parentBounds, float dt)            override;
    void render()                                          override;
    bool checkLeftClick(const Vec2f& mousePos)             override;
    bool checkHover(const Vec2f& mousePos)                 override;
    bool checkScroll(const Vec2f& mousePos, float delta)   override;
    void onDeactivate()                                    override;

    // Called by UILO event routing when this is the active interactible
    void handleTextInput(char32_t unicode)                                override;
    void handleKeyInput(SDL_Keycode key, bool shift, bool ctrl)           override;
    bool wantsTextInput() const override { return true; }

    std::string getString() const;
    void        setString(const std::string& s);
    bool        isFocused() const { return m_focused; }

private:
    Rectf         textArea()              const;
    float         lineHeight()            const;
    Vec2f         charScreenPos(size_t i) const;
    size_t        hitTestChar(Vec2f screenPos) const;
    void          ensureCursorVisible();
    bool          hasSelection()          const;
    void          deleteSelection();
    void          resetBlink();
    void          rebuildSfText();
    void          rebuildWrapped();        // builds m_wrappedDisplay + m_softWrapAt
    void          computeTextOrigin();
    std::u32string displayText()          const;

    // Soft-wrap index mapping helpers
    size_t        textToDisplay(size_t textIdx)   const;
    size_t        displayToText(size_t dispIdx)   const;

    size_t lineStart(size_t pos)  const;
    size_t lineEnd(size_t pos)    const;
    size_t wordLeft(size_t pos)   const;
    size_t wordRight(size_t pos)  const;

    TextboxOptions          m_options;
    std::u32string          m_text;
    size_t                  m_cursorPos     = 0;
    size_t                  m_anchorPos     = 0;   // same as cursor = no selection
    bool                    m_focused       = false;
    float                   m_blinkTimer    = 0.f;
    bool                    m_cursorVisible = true;
    float                   m_scrollOffsetX = 0.f;
    float                   m_scrollOffsetY = 0.f;
    float                   m_preferredX    = 0.f;  // preserved column x for up/down nav

    // Soft-wrap state (multiline+wrap mode)
    std::string             m_wrappedDisplay;           // UTF-8 display string with soft newlines
    std::u32string          m_displayU32;               // UTF-32 view of m_wrappedDisplay
    std::vector<Vec2f>      m_charPositions;            // per-codepoint position (size = m_displayU32.size()+1)
    std::vector<size_t>     m_softWrapAt;               // text indices where soft wrap was inserted
    float                   m_lastWrapWidth = 0.f;
    float                   m_lineHeightCache   = 0.f;
    float                   m_initialHeight     = 0.f;
    bool                    m_initialHeightSet  = false;
    bool                    m_mouseDown         = false;
    bool                    m_dragging          = false;  // true once mouse has moved past threshold
    bool                    m_needsCursorScroll = false;  // scroll to cursor only when it moved
    unsigned int            m_autoCharSize      = 0;      // resolved auto char size (0 = not yet computed)
    Vec2f                   m_mouseDownPos      = {0.f, 0.f};
    // TODO: font handle (FreeType) — deferred
    Rectf                   m_stableLineBounds;
    float                   m_lastScale     = 0.f;
    bool                    m_textDirty     = true;
    Vec2f                   m_textOrigin;
};

} // namespace uilo