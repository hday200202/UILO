#include "Textbox.hpp"
#include "../../utils/Resources.hpp"
#include "../../UILO.hpp"
#include "../../renderer/Renderer.hpp"

#include <SDL3/SDL.h>
#include <algorithm>
#include <cmath>
#include <limits>

namespace uilo {

/* UTF-8 / UTF-32 helpers */

/*
    u32ToUtf8(const std::u32string& s):
    - Params:   const std::u32string& s
    - Returns:  std::string
    - Desc:     Encodes UTF-32 to UTF-8. The text is held as UTF-32 so an index
                is a codepoint rather than a byte, which is what makes cursor
                movement and selection arithmetic simple; the renderer takes
                UTF-8, so a conversion happens at the boundary.
*/
static std::string u32ToUtf8(const std::u32string& s) {
    std::string r;
    r.reserve(s.size());
    for (char32_t c : s) {
        if (c < 0x80u) {
            r += static_cast<char>(c);
        } else if (c < 0x800u) {
            r += static_cast<char>(0xC0 | (c >> 6));
            r += static_cast<char>(0x80 | (c & 0x3F));
        } else if (c < 0x10000u) {
            r += static_cast<char>(0xE0 | (c >> 12));
            r += static_cast<char>(0x80 | ((c >> 6) & 0x3F));
            r += static_cast<char>(0x80 | (c & 0x3F));
        } else {
            r += static_cast<char>(0xF0 | (c >> 18));
            r += static_cast<char>(0x80 | ((c >> 12) & 0x3F));
            r += static_cast<char>(0x80 | ((c >> 6) & 0x3F));
            r += static_cast<char>(0x80 | (c & 0x3F));
        }
    }
    return r;
}

/*
    utf8ToU32(const std::string& s):
    - Params:   const std::string& s
    - Returns:  std::u32string
    - Desc:     Decodes UTF-8 to UTF-32. A truncated sequence at the end of the
                input is skipped rather than producing a partial codepoint.
*/
static std::u32string utf8ToU32(const std::string& s) {
    std::u32string r;
    size_t i = 0;
    while (i < s.size()) {
        uint8_t c = static_cast<uint8_t>(s[i]);
        char32_t cp = 0;
        if (c < 0x80u) {
            cp = c; i += 1;
        } else if (c < 0xE0u && i + 1 < s.size()) {
            cp = (static_cast<char32_t>(c & 0x1Fu) << 6) |
                  static_cast<char32_t>(static_cast<uint8_t>(s[i+1]) & 0x3Fu);
            i += 2;
        } else if (c < 0xF0u && i + 2 < s.size()) {
            cp = (static_cast<char32_t>(c & 0x0Fu) << 12) |
                 (static_cast<char32_t>(static_cast<uint8_t>(s[i+1]) & 0x3Fu) << 6) |
                  static_cast<char32_t>(static_cast<uint8_t>(s[i+2]) & 0x3Fu);
            i += 3;
        } else if (i + 3 < s.size()) {
            cp = (static_cast<char32_t>(c & 0x07u) << 18) |
                 (static_cast<char32_t>(static_cast<uint8_t>(s[i+1]) & 0x3Fu) << 12) |
                 (static_cast<char32_t>(static_cast<uint8_t>(s[i+2]) & 0x3Fu) << 6) |
                  static_cast<char32_t>(static_cast<uint8_t>(s[i+3]) & 0x3Fu);
            i += 4;
        } else {
            ++i;
        }
        if (cp) r += cp;
    }
    return r;
}

/*
    isWordChar(char32_t c):
    - Params:   char32_t c
    - Returns:  bool
    - Desc:     Whether a codepoint counts as part of a word for the ctrl-arrow
                jumps. Everything above ASCII is treated as a word character, so
                accented and non-Latin text is not split into fragments.
*/
static bool isWordChar(char32_t c) {
    return (c >= U'a' && c <= U'z') || (c >= U'A' && c <= U'Z') ||
           (c >= U'0' && c <= U'9') || c == U'_' || c > 127u;
}

/*
    shouldWrap(const TextboxOptions& opts):
    - Params:   const TextboxOptions& opts
    - Returns:  bool
    - Desc:     Whether soft wrapping is active. Password mode never wraps,
                since the masked string has no words to break on.
*/
static bool shouldWrap(const TextboxOptions& opts) {
    return opts.getMultiline() && opts.getWrap() && !opts.getPasswordMode();
}


Textbox::Textbox(Modifier modifier, TextboxOptions options, const std::string& name)
    : m_options(std::move(options))
{
    m_modifier = modifier;
    m_name     = name;
    m_type     = ElementType::TextBox;
}


/*
    autoGrows():
    - Params:   none
    - Returns:  bool
    - Desc:     Whether the box grows with its content instead of filling its
                slot.
*/
bool Textbox::autoGrows() const {
    /* A pixel height is a starting height the box may grow past -- the
       chat-input shape. */
    return m_options.getMultiline()
        && m_options.getWrap()
        && m_modifier.getHeight().isAbsolute();
}


/*
    maxScrollY(int lineCount, float lh):
    - Params:   int lineCount, float lh
    - Returns:  float
    - Desc:     How far the view may scroll vertically. A growing box only has
                something to scroll once maxResizeLines has capped it; a fill
                box scrolls whatever does not fit.
*/
float Textbox::maxScrollY(int lineCount, float lh) const {
    const int ml = m_options.getMaxResizeLines();
    if (autoGrows()) {
        /* The box grew to fit, so there is only something to scroll once
           maxResizeLines has capped it. */
        return (ml > 0 && lineCount > ml)
             ? static_cast<float>(lineCount - ml) * lh : 0.f;
    }
    /* Fixed slot: whatever does not fit is scrollable. */
    return std::max(0.f, static_cast<float>(lineCount) * lh - textArea().size.y);
}


/*
    resolvedFontPath():
    - Params:   none
    - Returns:  std::string
    - Desc:     The font to load, resolved the same way Text resolves one: a
                registered name becomes its path, a plain path is handed back
                unchanged.
*/
std::string Textbox::resolvedFontPath() const {
    /* Same resolution Text uses: a registered name becomes its path, a plain
       path is handed back unchanged. */
    return std::string(Resources::get().fontRegistry().resolve(m_options.getFontPath()));
}

/*
    gutterWidth():
    - Params:   none
    - Returns:  float
    - Desc:     Width of the line-number gutter, 0 when it is off or the box is
                single line. Reads a cached value, since the real width needs
                the font.
*/
float Textbox::gutterWidth() const {
    if (!m_options.getShowLineNumbers() || !m_options.getMultiline()) return 0.f;
    return m_gutterWidth;
}

/*
    textArea():
    - Params:   none
    - Returns:  Rectf
    - Desc:     The rectangle the text actually occupies: the bounds less the
                padding and the gutter. Everything else -- the caret, hit
                testing, wrap width, scrolling -- is measured against this, so
                insetting it here is all the gutter needs to be accounted for
                everywhere.
*/
Rectf Textbox::textArea() const {
    const float scale = m_uiloRef ? m_uiloRef->getScale() : 1.f;
    const float pl = m_options.getPaddingLeft()   * scale;
    const float pr = m_options.getPaddingRight()  * scale;
    const float pt = m_options.getPaddingTop()    * scale;
    const float pb = m_options.getPaddingBottom() * scale;
    const float gw = gutterWidth();
    return Rectf{
        { m_bounds.position.x + pl + gw, m_bounds.position.y + pt },
        { std::max(0.f, m_bounds.size.x - pl - pr - gw),
          std::max(0.f, m_bounds.size.y - pt - pb) }
    };
}

/*
    gutterArea():
    - Params:   none
    - Returns:  Rectf
    - Desc:     The strip the line numbers are drawn in, to the left of the text
                area.
*/
Rectf Textbox::gutterArea() const {
    const float scale = m_uiloRef ? m_uiloRef->getScale() : 1.f;
    const float pl = m_options.getPaddingLeft()   * scale;
    const float pt = m_options.getPaddingTop()    * scale;
    const float pb = m_options.getPaddingBottom() * scale;
    return Rectf{
        { m_bounds.position.x + pl, m_bounds.position.y + pt },
        { gutterWidth(), std::max(0.f, m_bounds.size.y - pt - pb) }
    };
}

/*
    recomputeGutterWidth(Renderer& renderer, float pxH):
    - Params:   Renderer& renderer, float pxH
    - Returns:  void
    - Desc:     Measures the gutter from the widest line number it will have to
                show, at the line-number character size, plus padding and
                whatever slack bold needs. Cached because textArea() is const
                and called from everywhere, and the measurement needs a
                renderer.
*/
void Textbox::recomputeGutterWidth(Renderer& renderer, float pxH) {
    if (!m_options.getShowLineNumbers() || !m_options.getMultiline()) {
        m_gutterWidth = 0.f;
        return;
    }

    size_t lines = 1;
    for (char32_t c : m_text) if (c == U'\n') ++lines;

    int digits = 1;
    for (size_t n = lines; n >= 10; n /= 10) ++digits;
    digits = std::max(digits, std::max(1, m_options.getLineNumberMinDigits()));

    const float scale = m_uiloRef ? m_uiloRef->getScale() : 1.f;
    const float lnPxH = m_options.hasLineNumberCharSize()
        ? static_cast<float>(m_options.getLineNumberCharSize()) * scale
        : pxH;

    Font font = renderer.loadFont(resolvedFontPath());
    float digitW = lnPxH * 0.6f;
    if (font.valid()) {
        const TextMetrics m = renderer.measureText("0", font, lnPxH);
        if (m.size.x > 0.f) digitW = m.size.x;
    }

    /* Bold re-draws each glyph nudged sideways, so a bold gutter is a shade
       wider than the measured digits and would otherwise clip its last column. */
    const float boldPad = m_options.getLineNumberBold()
                       || m_options.getCurrentLineNumberBold()
        ? std::max(1.f, lnPxH * 0.04f) : 0.f;

    const float pad = m_options.getLineNumberPadding() * scale;
    m_gutterWidth = digitW * static_cast<float>(digits) + boldPad + pad * 2.f;
}

/*
    logicalLineOfCursor():
    - Params:   none
    - Returns:  size_t
    - Desc:     Which logical line the cursor is on, counting hard newlines
                only, so a soft-wrapped line counts once.
*/
size_t Textbox::logicalLineOfCursor() const {
    size_t line = 0;
    const size_t end = std::min(m_cursorPos, m_text.size());
    for (size_t i = 0; i < end; ++i)
        if (m_text[i] == U'\n') ++line;
    return line;
}

/*
    lineHeight():
    - Params:   none
    - Returns:  float
    - Desc:     Height of one line. Uses the font's real ascent, descent and gap
                once the font has loaded, and falls back to an estimate from the
                character size before then.
*/
float Textbox::lineHeight() const {
    if (m_lineHeightCache > 0.f) return m_lineHeightCache;
    const float scale = m_uiloRef ? m_uiloRef->getScale() : 1.f;
    const unsigned int cs = m_options.hasCharSize() ? m_options.getCharSize()
                                                     : std::max(1u, m_autoCharSize);
    return static_cast<float>(cs) * scale * 1.2f;
}

/*
    displayText():
    - Params:   none
    - Returns:  std::u32string
    - Desc:     The text as it should appear, which is a run of asterisks in
                password mode and the text itself otherwise.
*/
std::u32string Textbox::displayText() const {
    if (m_options.getPasswordMode() && !m_text.empty())
        return std::u32string(m_text.size(), U'*');
    return m_text;
}

/*
    charScreenPos(size_t idx):
    - Params:   size_t idx
    - Returns:  Vec2f
    - Desc:     Where a text index sits on screen. The index is mapped through
                the soft wrap table first when wrapping is on, since inserted
                breaks make display and text indices diverge.
*/
Vec2f Textbox::charScreenPos(size_t idx) const {
    if (m_charPositions.empty()) return m_textOrigin;
    const size_t dispIdx = shouldWrap(m_options) ? textToDisplay(idx) : idx;
    const size_t clamped = std::min(dispIdx, m_charPositions.size() - 1);
    const Vec2f rel = m_charPositions[clamped];
    return { m_textOrigin.x + rel.x, m_textOrigin.y + rel.y };
}

/*
    hitTestChar(Vec2f screenPos):
    - Params:   Vec2f screenPos
    - Returns:  size_t
    - Desc:     The text index nearest a screen position, used to place the
                cursor on a click or drag. Resolved in two passes -- first the
                visual line, then the closest boundary along it -- so clicking
                past the end of a short line lands at that line's end rather
                than on whatever character happens to be nearest in a straight
                line.
*/
size_t Textbox::hitTestChar(Vec2f screenPos) const {
    if (m_charPositions.empty() || !m_uiloRef) return 0;
    const float lh      = std::max(1.f, lineHeight());
    const size_t dispN  = m_charPositions.size();

    /* Pass 1: find which visual line the click is on */
    float bestLineY    = m_textOrigin.y;
    float bestLineDist = std::numeric_limits<float>::max();
    for (size_t i = 0; i < dispN; ++i) {
        const float cy = m_textOrigin.y + m_charPositions[i].y;
        const float center = cy + lh * 0.5f;
        const float dy = std::abs(center - screenPos.y);
        if (dy < bestLineDist) { bestLineDist = dy; bestLineY = cy; }
    }

    /* Pass 2: nearest x on that line */
    size_t best = 0;
    float bestXDist = std::numeric_limits<float>::max();
    for (size_t i = 0; i < dispN; ++i) {
        const float cy = m_textOrigin.y + m_charPositions[i].y;
        if (std::abs(cy - bestLineY) > lh * 0.5f) continue;
        const float cx = m_textOrigin.x + m_charPositions[i].x;
        const float dx = std::abs(cx - screenPos.x);
        if (dx < bestXDist) { bestXDist = dx; best = i; }
    }

    return shouldWrap(m_options) ? displayToText(best) : best;
}


/*
    rebuildSfText():
    - Params:   none
    - Returns:  void
    - Desc:     Rebuilds everything derived from the text: the line height, the
                gutter width, the wrapped display string and the per-character
                positions. The gutter is measured before wrapping because it
                takes width off the text area and so changes where lines break.
*/
void Textbox::rebuildSfText() {
    m_textDirty = false;
    m_charPositions.clear();
    m_charPositions.push_back({0.f, 0.f});
    m_displayU32.clear();
    m_wrappedDisplay.clear();

    /* Bailing out before the font is available leaves the metrics unbuilt, so
       the rebuild has to stay pending. */
    if (!m_uiloRef) { m_textDirty = true; return; }
    auto& renderer = m_uiloRef->getRenderer();
    Font font = renderer.loadFont(resolvedFontPath());
    if (!font.valid()) { m_textDirty = true; return; }

    const float scale = m_uiloRef->getScale();
    const unsigned int cs = m_options.hasCharSize() ? m_options.getCharSize()
                                                     : std::max(1u, m_autoCharSize);
    const float pxH = static_cast<float>(cs) * scale;

    /* Cache real line height from the font. */
    {
        TextMetrics ref = renderer.measureText("A", font, pxH);
        m_lineHeightCache = ref.lineHeight();
    }

    /* Before wrapping: the gutter takes width off the text area, so the wrap
       width depends on it. */
    recomputeGutterWidth(renderer, pxH);

    /* Build display (UTF-32) — wrapped or raw. */
    if (shouldWrap(m_options)) {
        rebuildWrapped();
        m_lastWrapWidth = textArea().size.x;
    } else {
        m_displayU32 = displayText();
        m_wrappedDisplay = u32ToUtf8(m_displayU32);
        m_softWrapAt.clear();
    }

    m_charPositions = renderer.charPositions(m_wrappedDisplay, font, pxH);
    if (m_charPositions.empty()) m_charPositions.push_back({0.f, 0.f});
}

/* Build m_displayU32 (with soft '\n's) and m_softWrapAt from m_text. */
/*
    rebuildWrapped():
    - Params:   none
    - Returns:  void
    - Desc:     Builds the display string with soft breaks inserted, and records
                the text indices they were inserted at so display and text
                positions can be mapped back and forth. Breaks on word
                boundaries where it can and mid-word when a single word is wider
                than the line.
*/
void Textbox::rebuildWrapped() {
    m_softWrapAt.clear();
    m_displayU32.clear();
    if (!m_uiloRef) { m_displayU32 = m_text; m_wrappedDisplay = u32ToUtf8(m_displayU32); return; }
    auto& renderer = m_uiloRef->getRenderer();
    Font font = renderer.loadFont(resolvedFontPath());
    if (!font.valid()) {
        m_displayU32 = m_text;
        m_wrappedDisplay = u32ToUtf8(m_displayU32);
        return;
    }

    const float scale = m_uiloRef->getScale();
    const unsigned int cs = m_options.hasCharSize() ? m_options.getCharSize()
                                                     : std::max(1u, m_autoCharSize);
    const float pxH = static_cast<float>(cs) * scale;
    const float maxWidth = textArea().size.x;
    const std::u32string& src = m_text;

    if (maxWidth <= 0.f) {
        m_displayU32 = src;
        m_wrappedDisplay = u32ToUtf8(m_displayU32);
        return;
    }

    auto measureW = [&](const std::u32string& s) -> float {
        if (s.empty()) return 0.f;
        return renderer.measureText(u32ToUtf8(s), font, pxH).size.x;
    };

    std::u32string result;
    result.reserve(src.size() + 8);

    auto processParagraph = [&](size_t pStart, size_t pEnd) {
        std::u32string lineStr;
        size_t j = pStart;

        auto charWrap = [&](size_t from, size_t to) {
            for (size_t k = from; k < to; ++k) {
                std::u32string probe = lineStr + src[k];
                if (!lineStr.empty() && measureW(probe) > maxWidth) {
                    result += lineStr;
                    m_softWrapAt.push_back(k);
                    result += U'\n';
                    lineStr.clear();
                }
                lineStr += src[k];
            }
        };

        while (j < pEnd) {
            size_t wordEnd = j;
            while (wordEnd < pEnd && src[wordEnd] != U' ') ++wordEnd;
            size_t tokenEnd = wordEnd;
            while (tokenEnd < pEnd && src[tokenEnd] == U' ') ++tokenEnd;
            if (tokenEnd == j) ++tokenEnd;   /* stall guard */

            std::u32string token(src.begin() + (std::ptrdiff_t)j,
                                 src.begin() + (std::ptrdiff_t)tokenEnd);

            float tw = measureW(lineStr + token);
            if (tw <= maxWidth) {
                lineStr += token;
            } else {
                std::u32string wordOnly(src.begin() + (std::ptrdiff_t)j,
                                        src.begin() + (std::ptrdiff_t)wordEnd);
                float wordW = measureW(wordOnly);
                if (wordW <= maxWidth) {
                    if (!lineStr.empty()) {
                        result += lineStr;
                        m_softWrapAt.push_back(j);
                        result += U'\n';
                        lineStr.clear();
                    }
                    lineStr = token;
                } else {
                    if (!lineStr.empty()) {
                        result += lineStr;
                        m_softWrapAt.push_back(j);
                        result += U'\n';
                        lineStr.clear();
                    }
                    charWrap(j, wordEnd);
                    lineStr += std::u32string(src.begin() + (std::ptrdiff_t)wordEnd,
                                              src.begin() + (std::ptrdiff_t)tokenEnd);
                }
            }
            j = tokenEnd;
        }
        result += lineStr;
    };

    bool firstParagraph = true;
    size_t paraBegin = 0;
    for (size_t i = 0; i <= src.size(); ++i) {
        if (i == src.size() || src[i] == U'\n') {
            if (!firstParagraph) result += U'\n';
            firstParagraph = false;
            processParagraph(paraBegin, i);
            paraBegin = i + 1;
        }
    }

    m_displayU32     = std::move(result);
    m_wrappedDisplay = u32ToUtf8(m_displayU32);
}

/*
    computeTextOrigin():
    - Params:   none
    - Returns:  void
    - Desc:     Works out where the text block is drawn, from the alignment and
                the scroll offset. A growing box asking to be centred is the
                auto-grow case, so it centres against the height it was built at
                rather than the height it has now -- otherwise its first line
                would drift down as lines were added.
*/
void Textbox::computeTextOrigin() {
    if (m_charPositions.empty()) { m_textOrigin = textArea().position; return; }
    const Rectf area = textArea();
    const float lh   = lineHeight();

    /* Total height = (lines) * lh (excluding gap on the last line, but lh
       already includes lineGap which is fine for centering) Total width = max. */
    float maxX = 0.f, maxY = 0.f;
    for (auto& p : m_charPositions) {
        if (p.x > maxX) maxX = p.x;
        if (p.y > maxY) maxY = p.y;
    }
    const float totalH = maxY + lh;
    const float totalW = maxX;

    float ox;
    if (hasAlign(m_options.getTextAlignX(), Align::CenterX))
        ox = area.position.x + (area.size.x - totalW) * 0.5f - m_scrollOffsetX;
    else if (hasAlign(m_options.getTextAlignX(), Align::Right))
        ox = area.position.x + (area.size.x - totalW) - m_scrollOffsetX;
    else
        ox = area.position.x - m_scrollOffsetX;

    /* Vertical. */
    const Align alignY = m_options.getTextAlignY();
    float oy;
    if (autoGrows() && hasAlign(alignY, Align::CenterY)) {
        /* The box grows with its content, so centring against the height it
           has now would drift the first line downwards as lines are added. */
        const float sc          = m_uiloRef ? m_uiloRef->getScale() : 1.f;
        const float ptS         = m_options.getPaddingTop()    * sc;
        const float pbS         = m_options.getPaddingBottom() * sc;
        const float scaledInitH = m_initialHeight * sc;
        const float initAreaH   = m_initialHeightSet
                                ? std::max(lh, scaledInitH - ptS - pbS)
                                : lh;
        const float topOffset = (initAreaH - lh) * 0.5f;
        oy = area.position.y + topOffset - m_scrollOffsetY;
    } else if (hasAlign(alignY, Align::CenterY)) {
        oy = area.position.y + (area.size.y - totalH) * 0.5f - m_scrollOffsetY;
    } else if (hasAlign(alignY, Align::Bottom)) {
        oy = area.position.y + (area.size.y - totalH) - m_scrollOffsetY;
    } else {
        oy = area.position.y - m_scrollOffsetY;
    }

    m_textOrigin = { std::round(ox), std::round(oy) };
}

/*
    ensureCursorVisible():
    - Params:   none
    - Returns:  void
    - Desc:     Scrolls just enough to bring the cursor inside the text area, on
                whichever axis it left. Called only when the cursor actually
                moved, so typing does not fight a user's own scrolling.
*/
void Textbox::ensureCursorVisible() {
    if (m_charPositions.empty()) return;
    const Rectf area = textArea();
    const float lh   = lineHeight();
    const Vec2f cp   = charScreenPos(m_cursorPos);

    if (!m_options.getMultiline()) {
        const float right = area.position.x + area.size.x;
        if (cp.x < area.position.x) {
            m_scrollOffsetX -= area.position.x - cp.x;
            m_scrollOffsetX  = std::max(0.f, m_scrollOffsetX);
            computeTextOrigin();
        } else if (cp.x > right) {
            m_scrollOffsetX += cp.x - right;
            computeTextOrigin();
        }
    }

    if (m_options.getMultiline()) {
        const float bottom = area.position.y + area.size.y;
        if (cp.y < area.position.y) {
            m_scrollOffsetY -= area.position.y - cp.y;
            m_scrollOffsetY  = std::max(0.f, m_scrollOffsetY);
            computeTextOrigin();
        } else if (cp.y + lh > bottom) {
            m_scrollOffsetY += (cp.y + lh) - bottom;
            computeTextOrigin();
        }
    }
}

/*
    textToDisplay(size_t textIdx):
    - Params:   size_t textIdx
    - Returns:  size_t
    - Desc:     Maps a text index to its position in the wrapped display string,
                adding one for each soft break inserted before it.
*/
size_t Textbox::textToDisplay(size_t textIdx) const {
    size_t extra = 0;
    for (size_t wrapPos : m_softWrapAt) {
        if (textIdx >= wrapPos) ++extra;
        else break;
    }
    return textIdx + extra;
}

/*
    displayToText(size_t dispIdx):
    - Params:   size_t dispIdx
    - Returns:  size_t
    - Desc:     The inverse mapping, from a display index back to the text.
*/
size_t Textbox::displayToText(size_t dispIdx) const {
    size_t extra = 0;
    for (size_t wrapPos : m_softWrapAt) {
        if (dispIdx > wrapPos + extra) ++extra;
        else break;
    }
    return dispIdx >= extra ? dispIdx - extra : 0;
}


/*
    hasSelection():
    - Params:   none
    - Returns:  bool
    - Desc:     Whether anything is selected, which is simply the cursor and the
                anchor sitting at different positions.
*/
bool Textbox::hasSelection() const {
    return m_cursorPos != m_anchorPos;
}

/*
    deleteSelection():
    - Params:   none
    - Returns:  void
    - Desc:     Removes the selected range and collapses the cursor to where it
                began.
*/
void Textbox::deleteSelection() {
    const size_t lo = std::min(m_cursorPos, m_anchorPos);
    const size_t hi = std::max(m_cursorPos, m_anchorPos);
    m_text.erase(lo, hi - lo);
    m_cursorPos = m_anchorPos = lo;
    m_textDirty = true;
    m_needsCursorScroll = true;
    if (m_options.getOnStringChanged())
        m_options.getOnStringChanged()(getString());
}

/*
    resetBlink():
    - Params:   none
    - Returns:  void
    - Desc:     Shows the caret and restarts its blink timer, so it is solid at
                the moment of any edit or cursor move rather than possibly mid-
                blink.
*/
void Textbox::resetBlink() {
    m_blinkTimer    = 0.f;
    m_cursorVisible = true;
    m_dirty = true;
}

/*
    lineStart(size_t pos):
    - Params:   size_t pos
    - Returns:  size_t
    - Desc:     Index of the first character on the line containing a position.
*/
size_t Textbox::lineStart(size_t pos) const {
    if (pos == 0) return 0;
    size_t i = pos;
    while (i > 0 && m_text[i - 1] != U'\n') --i;
    return i;
}

/*
    lineEnd(size_t pos):
    - Params:   size_t pos
    - Returns:  size_t
    - Desc:     Index just past the last character on the line containing a
                position.
*/
size_t Textbox::lineEnd(size_t pos) const {
    size_t i = pos;
    while (i < m_text.size() && m_text[i] != U'\n') ++i;
    return i;
}

/*
    wordLeft(size_t pos):
    - Params:   size_t pos
    - Returns:  size_t
    - Desc:     The next word boundary to the left, skipping any run of
                separators first so a ctrl-left from after a space lands at the
                start of the previous word rather than on the space.
*/
size_t Textbox::wordLeft(size_t pos) const {
    if (pos == 0) return 0;
    size_t i = pos - 1;
    while (i > 0 && !isWordChar(m_text[i])) --i;
    while (i > 0 && isWordChar(m_text[i - 1])) --i;
    return i;
}

/*
    wordRight(size_t pos):
    - Params:   size_t pos
    - Returns:  size_t
    - Desc:     The next word boundary to the right, by the mirror of the same
                rule.
*/
size_t Textbox::wordRight(size_t pos) const {
    size_t i = pos;
    const size_t n = m_text.size();
    while (i < n && !isWordChar(m_text[i])) ++i;
    while (i < n && isWordChar(m_text[i])) ++i;
    return i;
}

/* Public string API */

/*
    getString():
    - Params:   none
    - Returns:  std::string
    - Desc:     The current text as UTF-8. This is the real text, not the masked
                or soft-wrapped display form.
*/
std::string Textbox::getString() const {
    return u32ToUtf8(m_text);
}

/*
    setString(const std::string& s):
    - Params:   const std::string& s
    - Returns:  void
    - Desc:     Replaces the text and clamps the cursor and anchor into the new
                length, so a shorter string cannot leave them dangling past the
                end.
*/
void Textbox::setString(const std::string& s) {
    m_text      = utf8ToU32(s);
    m_cursorPos = m_anchorPos = std::min(m_cursorPos, m_text.size());
    m_textDirty = true;
    m_scrollOffsetX = m_scrollOffsetY = 0.f;
}


/*
    update(Rectf& parentBounds, float dt):
    - Params:   Rectf& parentBounds, float dt
    - Returns:  void
    - Desc:     Resolves bounds, rebuilds the text when anything it depends on
                has moved, sizes the box, advances the caret blink and services
                a drag selection. Height is handled one of two ways: a pixel-
                sized box grows with its content and publishes that height back
                to its parent, while a percent-sized one fills the slot it was
                given and scrolls instead -- growing the latter would mean
                rewriting its declared height to pixels, which would sever it
                from the layout and stop it tracking a window resize.
*/
void Textbox::update(Rectf& parentBounds, float dt) {
    resize(parentBounds);

    const float scale = m_uiloRef ? m_uiloRef->getScale() : 1.f;

    if (!m_options.hasCharSize()) {
        const float baseH    = m_initialHeightSet ? m_initialHeight : m_bounds.size.y / scale;
        const unsigned int autoCs = std::max(1u, static_cast<unsigned int>(baseH * 0.6f));
        if (autoCs != m_autoCharSize) {
            m_autoCharSize = autoCs;
            m_textDirty    = true;
        }
    }

    if (scale != m_lastScale) {
        m_lastScale     = scale;
        m_textDirty     = true;
        m_scrollOffsetX = m_scrollOffsetY = 0.f;
    }

    /* Re-wrap if the available text width has changed. */
    if (shouldWrap(m_options)) {
        const float wrapW = textArea().size.x;
        if (std::abs(wrapW - m_lastWrapWidth) > 0.5f) m_textDirty = true;
    }

    if (m_textDirty) {
        m_dirty = true;
        rebuildSfText();
    }

    /* Height: grow to fit the content, or fill the slot the parent gave us. */
    if (autoGrows()) {
        if (!m_initialHeightSet) {
            m_initialHeight    = m_bounds.size.y / scale;
            m_initialHeightSet = true;
        }
        int lineCount = 1;
        for (char c : m_wrappedDisplay) if (c == '\n') ++lineCount;
        const float lh = lineHeight();
        const float pt = m_options.getPaddingTop()    * scale;
        const float pb = m_options.getPaddingBottom() * scale;
        const int   ml = m_options.getMaxResizeLines();
        const float wantedH  = static_cast<float>(lineCount) * lh + pt + pb;
        const float clampedH = (ml > 0 && lineCount > ml)
                               ? static_cast<float>(ml) * lh + pt + pb : wantedH;
        m_bounds.size.y = std::max(m_initialHeight * scale, clampedH);

        /* Clamp vertical scroll so empty lines never appear below the text
           after the user deletes content. */
        m_scrollOffsetY = std::max(0.f, std::min(m_scrollOffsetY, maxScrollY(lineCount, lh)));

        /* Publish the grown height back so the parent reserves room for it. */
        const float unscaled = m_bounds.size.y / scale;
        if (m_modifier.getHeight().value != unscaled)
            m_modifier.setHeight(Dimension{unscaled, false});
    } else if (m_options.getMultiline()) {
        /* Filling a percent slot: resize() already sized us to it, so leave
           the bounds alone and just keep the scroll offset inside the new. */
        int lineCount = 1;
        for (char c : m_wrappedDisplay) if (c == '\n') ++lineCount;
        m_scrollOffsetY = std::max(0.f,
            std::min(m_scrollOffsetY, maxScrollY(lineCount, lineHeight())));
    }

    /* Drag-select */
    if (m_mouseDown) {
        float mx, my;
        uint32_t btns = SDL_GetMouseState(&mx, &my);
        if (!(btns & SDL_BUTTON_MASK(SDL_BUTTON_LEFT))) {
            m_mouseDown = false;
            m_dragging  = false;
        } else if (m_uiloRef) {
            const Vec2f mp = m_uiloRef->getMousePosition();
            const float dx = mp.x - m_mouseDownPos.x;
            const float dy = mp.y - m_mouseDownPos.y;
            if (!m_dragging && (dx * dx + dy * dy) > 9.f)
                m_dragging = true;
            if (m_dragging) {
                const size_t idx = hitTestChar(mp);
                if (idx != m_cursorPos) {
                    m_cursorPos = idx;
                    resetBlink();
                    m_needsCursorScroll = true;
                }
            }
        }
    }

    computeTextOrigin();

    if (m_needsCursorScroll) {
        ensureCursorVisible();
        m_needsCursorScroll = false;
    }

    if (m_focused) {
        m_blinkTimer += dt;
        const float half = m_options.getBlinkRate() * 0.5f;
        if (m_blinkTimer >= half) {
            m_blinkTimer   -= half;
            m_cursorVisible = !m_cursorVisible;
            m_dirty = true;
        }
    }
}


/*
    render():
    - Params:   none
    - Returns:  void
    - Desc:     Draws the background and focus outline, the line-number gutter,
                then the selection, the text and the caret, all clipped to the
                text area. The placeholder replaces the text when the box is
                empty and unfocused. The caret is clamped so its full width
                stays inside the area: centred on the insertion point it would
                otherwise be sliced in half at the start of every line.
*/
void Textbox::render() {
    if (!m_modifier.getVisible()) { m_dirty = false; return; }
    m_dirty = false;
    if (!m_uiloRef) return;
    auto& renderer = m_uiloRef->getRenderer();

    /* Background + optional outline */
    const float bgScale = m_uiloRef ? m_uiloRef->getScale() : 1.f;
    const float rounding = m_options.getRounding() * bgScale;
    Color bg = resolveColor(m_options.getBackgroundColorRole(), m_options.getBackgroundColor());
    Color outline = m_focused
        ? resolveColor(m_options.getOutlineColorRole(), m_options.getOutlineColor())
        : Color{0,0,0,0};
    float outlineT = m_focused ? m_options.getOutlineThickness() * bgScale : 0.f;
    if (rounding > 0.f) {
        renderer.draw(RoundedRect{
            m_bounds.position, m_bounds.size, rounding, 8,
            bg, outline, outlineT});
    } else {
        renderer.draw(Rect{m_bounds.position, m_bounds.size, bg, outline, outlineT});
    }

    Font font = renderer.loadFont(resolvedFontPath());
    if (!font.valid()) return;

    const float scale = m_uiloRef->getScale();
    const unsigned int cs = m_options.hasCharSize() ? m_options.getCharSize()
                                                    : std::max(1u, m_autoCharSize);
    const float pxH = (float)cs * scale;

    renderLineNumbers(renderer, pxH);

    const Rectf area = textArea();
    renderer.pushScissor(area);

    const bool showPlaceholder = m_text.empty() && !m_focused;
    Color textColor = showPlaceholder
        ? resolveColor(m_options.getPlaceholderColorRole(), m_options.getPlaceholderColor())
        : resolveColor(m_options.getTextColorRole(),        m_options.getTextColor());

    if (showPlaceholder) {
        const std::string& ph = m_options.getPlaceholder();
        if (!ph.empty()) {
            TextMetrics pm = renderer.measureText(ph, font, pxH);
            Vec2f origin = area.position;
            if (hasAlign(m_options.getTextAlignX(), Align::CenterX))
                origin.x += (area.size.x - pm.size.x) * 0.5f;
            else if (hasAlign(m_options.getTextAlignX(), Align::Right))
                origin.x += (area.size.x - pm.size.x);
            if (!m_options.getMultiline()) {
                if (hasAlign(m_options.getTextAlignY(), Align::CenterY))
                    origin.y += (area.size.y - pm.size.y) * 0.5f;
                else if (hasAlign(m_options.getTextAlignY(), Align::Bottom))
                    origin.y += (area.size.y - pm.size.y);
            }
            renderer.drawText(ph, { std::round(origin.x), std::round(origin.y) },
                              font, pxH, textColor,
                              TextStyle{m_options.getBold(), m_options.getItalic()});
        }
    } else {
        /* Selection rects (per character; newline fills to area right edge). */
        const float lh = lineHeight();
        if (m_focused && hasSelection()) {
            const size_t lo = std::min(m_cursorPos, m_anchorPos);
            const size_t hi = std::max(m_cursorPos, m_anchorPos);
            const Color  selCol = resolveColor(m_options.getSelectionColorRole(), m_options.getSelectionColor());
            for (size_t i = lo; i < hi; ++i) {
                Vec2f p0 = charScreenPos(i);
                Vec2f p1 = charScreenPos(i + 1);
                float x0 = p0.x;
                float x1 = p1.x;
                float y  = p0.y;
                bool isNewlineSel =
                    (i < m_text.size() && m_text[i] == U'\n') || (p1.y > p0.y + 0.5f);
                if (isNewlineSel) {
                    x1 = area.position.x + area.size.x;
                }
                if (x1 < x0) std::swap(x0, x1);
                renderer.draw(Rect{
                    { x0, y },
                    { std::max(1.f, x1 - x0), lh },
                    selCol
                });
            }
        }

        if (!m_wrappedDisplay.empty()) {
            renderer.drawText(m_wrappedDisplay, m_textOrigin, font, pxH, textColor,
                              TextStyle{m_options.getBold(), m_options.getItalic()});
        }

        /* Caret */
        if (m_focused && m_cursorVisible) {
            Vec2f cp = charScreenPos(m_cursorPos);
            const float cw    = std::max(1.f, m_options.getCursorWidth() * scale);
            const float halfW = cw * 0.5f;

            /* The caret is centred on the insertion point, so at either edge
               of the text area half of it would fall outside the scissor. */
            const float minX = area.position.x + halfW;
            const float maxX = area.position.x + area.size.x - halfW;
            const float cx   = std::round(std::clamp(cp.x, minX, std::max(minX, maxX)));

            renderer.draw(Line{
                { cx, cp.y },
                { cx, cp.y + lh },
                cw,
                resolveColor(m_options.getCursorColorRole(), m_options.getCursorColor()) });
        }
    }

    renderer.popScissor();
}

/*
    renderLineNumbers(Renderer& renderer, float pxH):
    - Params:   Renderer& renderer, float pxH
    - Returns:  void
    - Desc:     Draws the gutter and one number per logical line, so a soft-
                wrapped line is numbered once and its continuation rows are left
                blank. Numbers may be smaller than the body text, in which case
                they are baseline-aligned with it rather than sitting at the top
                of the line box. The cursor's line can take its own colour and
                style.
*/
void Textbox::renderLineNumbers(Renderer& renderer, float pxH) {
    if (gutterWidth() <= 0.f) return;

    Font font = renderer.loadFont(resolvedFontPath());
    if (!font.valid()) return;

    const Rectf gutter = gutterArea();
    const Color gbg = resolveColor(m_options.getLineNumberBackgroundColorRole(),
                                   m_options.getLineNumberBackgroundColor());
    if (gbg.a > 0)
        renderer.draw(Rect{gutter.position, gutter.size, gbg});

    const Color base = resolveColor(m_options.getLineNumberColorRole(),
                                    m_options.getLineNumberColor());
    const Color currentLit = m_options.getCurrentLineNumberColor();
    const bool  hasCurrent = currentLit.a > 0
                          || !m_options.getCurrentLineNumberColorRole().empty();
    const Color current = hasCurrent
        ? resolveColor(m_options.getCurrentLineNumberColorRole(), currentLit)
        : base;

    const float scale  = m_uiloRef ? m_uiloRef->getScale() : 1.f;
    const float pad    = m_options.getLineNumberPadding() * scale;
    const float lh     = lineHeight();
    const size_t curLine = logicalLineOfCursor();

    const TextStyle baseStyle{m_options.getLineNumberBold(),
                              m_options.getLineNumberItalic()};
    const TextStyle curStyle {m_options.getCurrentLineNumberBold(),
                              m_options.getCurrentLineNumberItalic()};

    /* Numbers may be drawn smaller than the editor text. */
    const float lnPxH = m_options.hasLineNumberCharSize()
        ? static_cast<float>(m_options.getLineNumberCharSize()) * scale
        : pxH;
    float baselineShift = 0.f;
    if (lnPxH != pxH) {
        const float mainAsc = renderer.measureText("0", font, pxH).ascent;
        const float lnAsc   = renderer.measureText("0", font, lnPxH).ascent;
        baselineShift = mainAsc - lnAsc;
    }

    renderer.pushScissor(gutter);

    /* One number per logical line, placed at the y of that line's first display
       row, so a soft-wrapped line is numbered once rather than per row. */
    size_t line = 0;
    size_t textIdx = 0;
    const size_t n = m_text.size();
    while (true) {
        const Vec2f p = charScreenPos(textIdx);

        if (p.y + lh >= gutter.position.y && p.y <= gutter.position.y + gutter.size.y) {
            const bool        isCur = (line == curLine);
            const TextStyle   style = isCur ? curStyle : baseStyle;
            const std::string label = std::to_string(line + 1);
            const TextMetrics m     = renderer.measureText(label, font, lnPxH);

            /* Bold widens the drawn run without changing the measurement, so a
               right-aligned bold number needs that slack or it overhangs. */
            const float boldPad = style.bold ? std::max(1.f, lnPxH * 0.04f) : 0.f;

            float x;
            if (hasAlign(m_options.getLineNumberAlign(), Align::Right))
                x = gutter.position.x + gutter.size.x - pad - m.size.x - boldPad;
            else if (hasAlign(m_options.getLineNumberAlign(), Align::CenterX))
                x = gutter.position.x + (gutter.size.x - m.size.x - boldPad) * 0.5f;
            else
                x = gutter.position.x + pad;

            renderer.drawText(label,
                              { std::round(x), std::round(p.y + baselineShift) },
                              font, lnPxH,
                              isCur ? current : base,
                              style);
        }

        /* Advance to the character after the next hard newline. */
        size_t nl = textIdx;
        while (nl < n && m_text[nl] != U'\n') ++nl;
        if (nl >= n) break;
        textIdx = nl + 1;
        ++line;
    }

    renderer.popScissor();
}


/*
    checkLeftClick(const Vec2f& mousePos):
    - Params:   const Vec2f& mousePos
    - Returns:  bool -- true when the click landed on the box
    - Desc:     Takes focus and places the cursor where the click landed,
                extending the selection instead when shift is held. Also arms a
                drag, so a press and sweep selects a range.
*/
bool Textbox::checkLeftClick(const Vec2f& mousePos) {
    if (!m_bounds.contains(mousePos)) return false;
    m_uiloRef->setCurrInteractible(this);
    m_focused       = true;
    m_mouseDown     = true;
    m_dragging      = false;
    m_mouseDownPos  = mousePos;
    resetBlink();
    const bool shiftHeld = (SDL_GetModState() & SDL_KMOD_SHIFT) != 0;
    const size_t idx = hitTestChar(mousePos);
    m_cursorPos = idx;
    if (!shiftHeld) m_anchorPos = idx;
    m_preferredX = charScreenPos(idx).x;
    return true;
}

/*
    checkHover(const Vec2f& mousePos):
    - Params:   const Vec2f& mousePos
    - Returns:  bool -- true when the pointer is over the box
    - Desc:     Asks for the text cursor while the pointer is inside.
*/
bool Textbox::checkHover(const Vec2f& mousePos) {
    if (m_bounds.contains(mousePos) && m_uiloRef)
        m_uiloRef->requestCursor(CursorType::Text, 1);
    return Element::checkHover(mousePos);
}

/*
    onDeactivate():
    - Params:   none
    - Returns:  void
    - Desc:     Releases focus when something else is clicked, ending any drag
                and hiding the caret.
*/
void Textbox::onDeactivate() {
    m_focused       = false;
    m_cursorVisible = false;
    m_mouseDown     = false;
    m_dragging      = false;
    m_dirty = true;
}

/*
    checkScroll(const Vec2f& mousePos, float delta, bool precise, bool momentum):
    - Params:   const Vec2f& mousePos, float delta, bool precise, bool momentum
    - Returns:  bool -- true when the box consumed the event
    - Desc:     Scrolls a multiline box, converting the delta exactly the way a
                scrollable Column does so the two feel identical under one
                gesture. The offset is continuous rather than snapped to whole
                lines, which is what lets a trackpad's momentum tail read as
                smooth instead of stepping. Declines when nothing overflows, so
                a wheel over a short editor scrolls the page behind it instead.
*/
bool Textbox::checkScroll(
    const Vec2f& mousePos,
    float delta,
    bool precise,
    bool /*momentum*/
) {
    if (!m_bounds.contains(mousePos)) return false;
    if (!m_options.getMultiline()) return false;

    const float lh = lineHeight();
    if (lh <= 0.f) return false;
    int lineCount = 1;
    for (char c : m_wrappedDisplay) if (c == '\n') ++lineCount;

    /* Nothing overflowing means nothing to scroll, and the event belongs to
       whatever is behind -- a page that scrolls past a short editor. */
    const float maxScroll = maxScrollY(lineCount, lh);
    if (maxScroll <= 0.f) return false;

    /* Same conversion a scrollable Column uses, so one gesture moves a textbox
       and a container by the same distance. A trackpad's precise pixel delta is
       scaled differently from a wheel's discrete step, since the OS already
       supplies the momentum tail for the former. */
    const float speed = m_options.getScrollSpeed();
    const float step  = precise ? 30.f * (speed / 40.f) : speed;

    const float next = std::clamp(m_scrollOffsetY - delta * step, 0.f, maxScroll);
    if (next != m_scrollOffsetY) {
        m_scrollOffsetY = next;
        m_dirty = true;
    }
    return true;
}


/*
    handleTextInput(char32_t c):
    - Params:   char32_t c
    - Returns:  void
    - Desc:     Inserts a typed codepoint, replacing the selection first.
                Control characters are ignored here; the ones that mean
                something arrive through handleKeyInput instead.
*/
void Textbox::handleTextInput(char32_t c) {
    if (!m_focused) return;
    if (c < 32u || c == 127u) return;
    if (hasSelection()) deleteSelection();
    const int maxLen = m_options.getMaxLength();
    if (maxLen > 0 && static_cast<int>(m_text.size()) >= maxLen) return;
    m_text.insert(m_text.begin() + static_cast<std::ptrdiff_t>(m_cursorPos), c);
    ++m_cursorPos;
    m_anchorPos = m_cursorPos;
    m_textDirty = true;
    resetBlink();
    m_needsCursorScroll = true;
    if (m_options.getOnStringChanged())
        m_options.getOnStringChanged()(getString());
}

/*
    insertTab():
    - Params:   none
    - Returns:  void
    - Desc:     Inserts one indent as spaces. Tab always inserts spaces rather
                than a literal tab character, because the renderer has no tab-
                advance logic and a real one would draw as a missing-glyph box.
                Ignored on a single-line box, where indenting means nothing.
*/
void Textbox::insertTab() {
    /* A single-line box is a field, not an editor: indenting it makes no
       sense, and swallowing Tab there would also take away the key a future. */
    if (!m_options.getMultiline()) return;

    const int width = std::max(1, m_options.getTabWidth());
    if (hasSelection()) deleteSelection();

    const int maxLen = m_options.getMaxLength();
    int room = width;
    if (maxLen > 0) {
        room = std::min(width, maxLen - static_cast<int>(m_text.size()));
        if (room <= 0) return;
    }

    m_text.insert(m_cursorPos, static_cast<size_t>(room), U' ');
    m_cursorPos += static_cast<size_t>(room);
    m_anchorPos  = m_cursorPos;
    m_textDirty  = true;
    m_needsCursorScroll = true;
    resetBlink();
    if (m_options.getOnStringChanged()) m_options.getOnStringChanged()(getString());
}

/*
    handleKeyInput(SDL_Keycode key, bool shift, bool ctrl, bool gui):
    - Params:   SDL_Keycode key, bool shift, bool ctrl, bool gui
    - Returns:  void
    - Desc:     Handles every key that is not plain typing: cursor movement with
                the arrows, Home and End, word jumps with ctrl, deletion, Tab,
                Enter, Escape, and the clipboard and select-all shortcuts. Shift
                extends the selection by leaving the anchor where it is; without
                it the anchor follows the cursor and the selection collapses.
    - Word jumps stay on Control alone. Only the clipboard and select-all
      shortcuts accept Command as well, which is what a Mac user expects.
*/
void Textbox::handleKeyInput(SDL_Keycode key, bool shift, bool ctrl, bool gui) {
    if (!m_focused) return;

    const bool   shortcut = isShortcutModifier(ctrl, gui);
    const size_t n        = m_text.size();

    auto moveCursor = [&](size_t newPos) {
        m_cursorPos = newPos;
        if (!shift) m_anchorPos = newPos;
        resetBlink();
    };

    switch (key) {
        case SDLK_LEFT:
            if (!shift && hasSelection()) moveCursor(std::min(m_cursorPos, m_anchorPos));
            else if (m_cursorPos > 0) moveCursor(ctrl ? wordLeft(m_cursorPos) : m_cursorPos - 1);
            m_preferredX = charScreenPos(m_cursorPos).x;
            break;
        case SDLK_RIGHT:
            if (!shift && hasSelection()) moveCursor(std::max(m_cursorPos, m_anchorPos));
            else if (m_cursorPos < n) moveCursor(ctrl ? wordRight(m_cursorPos) : m_cursorPos + 1);
            m_preferredX = charScreenPos(m_cursorPos).x;
            break;
        case SDLK_UP:
            if (m_options.getMultiline()) {
                Vec2f cp = charScreenPos(m_cursorPos);
                moveCursor(hitTestChar({ m_preferredX, cp.y - lineHeight() * 0.5f }));
            }
            break;
        case SDLK_DOWN:
            if (m_options.getMultiline()) {
                Vec2f cp = charScreenPos(m_cursorPos);
                moveCursor(hitTestChar({ m_preferredX, cp.y + lineHeight() * 1.5f }));
            }
            break;
        case SDLK_HOME:
            moveCursor(ctrl ? 0 : lineStart(m_cursorPos));
            m_preferredX = charScreenPos(m_cursorPos).x;
            break;
        case SDLK_END:
            moveCursor(ctrl ? n : lineEnd(m_cursorPos));
            m_preferredX = charScreenPos(m_cursorPos).x;
            break;
        case SDLK_BACKSPACE:
            if (hasSelection()) deleteSelection();
            else if (m_cursorPos > 0) {
                size_t newPos = ctrl ? wordLeft(m_cursorPos) : m_cursorPos - 1;
                m_text.erase(newPos, m_cursorPos - newPos);
                m_cursorPos = m_anchorPos = newPos;
                m_textDirty = true;
            }
            resetBlink();
            if (m_options.getOnStringChanged()) m_options.getOnStringChanged()(getString());
            break;
        case SDLK_DELETE:
            if (hasSelection()) deleteSelection();
            else if (m_cursorPos < n) {
                size_t end = ctrl ? wordRight(m_cursorPos) : m_cursorPos + 1;
                m_text.erase(m_cursorPos, end - m_cursorPos);
                m_textDirty = true;
            }
            resetBlink();
            if (m_options.getOnStringChanged()) m_options.getOnStringChanged()(getString());
            break;
        case SDLK_TAB:
            insertTab();
            m_preferredX = charScreenPos(m_cursorPos).x;
            break;
        case SDLK_RETURN:
        case SDLK_KP_ENTER:
            if (m_options.getMultiline()) {
                if (hasSelection()) deleteSelection();
                const int maxLen = m_options.getMaxLength();
                if (maxLen <= 0 || static_cast<int>(m_text.size()) < maxLen) {
                    m_text.insert(m_text.begin() + static_cast<std::ptrdiff_t>(m_cursorPos), U'\n');
                    ++m_cursorPos;
                    m_anchorPos = m_cursorPos;
                    m_textDirty = true;
                    if (m_options.getOnStringChanged()) m_options.getOnStringChanged()(getString());
                }
            } else {
                if (m_options.getOnEnterPressed()) m_options.getOnEnterPressed()(getString());
            }
            resetBlink();
            m_preferredX = charScreenPos(m_cursorPos).x;
            break;
        case SDLK_ESCAPE:
            if (m_uiloRef) m_uiloRef->setCurrInteractible(nullptr);
            return;
        case SDLK_A:
            if (shortcut) { m_anchorPos = 0; m_cursorPos = n; resetBlink(); }
            break;
        case SDLK_C:
            if (shortcut && hasSelection()) {
                const size_t lo = std::min(m_cursorPos, m_anchorPos);
                const size_t hi = std::max(m_cursorPos, m_anchorPos);
                SDL_SetClipboardText(u32ToUtf8(m_text.substr(lo, hi - lo)).c_str());
            }
            break;
        case SDLK_X:
            if (shortcut && hasSelection()) {
                const size_t lo = std::min(m_cursorPos, m_anchorPos);
                const size_t hi = std::max(m_cursorPos, m_anchorPos);
                SDL_SetClipboardText(u32ToUtf8(m_text.substr(lo, hi - lo)).c_str());
                deleteSelection();
                if (m_options.getOnStringChanged()) m_options.getOnStringChanged()(getString());
            }
            break;
        case SDLK_V:
            if (shortcut) {
                const char* raw = SDL_GetClipboardText();
                if (raw && *raw) {
                    std::u32string pasted = utf8ToU32(raw);
                    SDL_free(const_cast<char*>(raw));
                    if (!pasted.empty()) {
                        if (hasSelection()) deleteSelection();
                        const int maxLen = m_options.getMaxLength();
                        if (maxLen > 0) {
                            size_t room = static_cast<size_t>(maxLen) > m_text.size()
                                        ? static_cast<size_t>(maxLen) - m_text.size() : 0u;
                            if (pasted.size() > room) pasted.resize(room);
                        }
                        m_text.insert(m_cursorPos, pasted);
                        m_cursorPos += pasted.size();
                        m_anchorPos  = m_cursorPos;
                        m_textDirty  = true;
                        resetBlink();
                        if (m_options.getOnStringChanged()) m_options.getOnStringChanged()(getString());
                    }
                }
            }
            break;
        default:
            break;
    }
    m_needsCursorScroll = true;
}

} // namespace uilo
