#include "Textbox.hpp"
#include "../../UILO.hpp"

#include <algorithm>

namespace uilo {

// ---------------------------------------------------------------------------
// UTF-8 / UTF-32 helpers
// ---------------------------------------------------------------------------

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

// findCharacterPos is the correct high-level API for cursor-position queries.
// getShapedGlyphs() uses byte-offset clusters rather than codepoint indices and
// would require a full byte↔codepoint mapping table to replace it safely here.
// We suppress the deprecation warning rather than introduce that complexity.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
static sf::Vector2f glyphPos(const sf::Text& text, size_t idx) {
    return text.findCharacterPos(idx);
}
#pragma GCC diagnostic pop

static bool isWordChar(char32_t c) {
    return (c >= U'a' && c <= U'z') || (c >= U'A' && c <= U'Z') ||
           (c >= U'0' && c <= U'9') || c == U'_' || c > 127u;
}

static bool shouldWrap(const TextboxOptions& opts) {
    return opts.getMultiline() && opts.getWrap() && !opts.getPasswordMode();
}

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

Textbox::Textbox(Modifier modifier, TextboxOptions options, const std::string& name)
    : m_options(std::move(options))
{
    m_modifier = modifier;
    m_name     = name;
    m_type     = ElementType::TextBox;

    if (m_options.getFontRef()) {
        m_fontPtr = m_options.getFontRef();
    } else if (!m_options.getFontPath().empty()) {
        if (m_ownedFont.openFromFile(m_options.getFontPath()))
            m_fontPtr = &m_ownedFont;
    }
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

sf::FloatRect Textbox::textArea() const {
    const float scale = m_uiloRef ? m_uiloRef->getScale() : 1.f;
    const float pl = m_options.getPaddingLeft()   * scale;
    const float pr = m_options.getPaddingRight()  * scale;
    const float pt = m_options.getPaddingTop()    * scale;
    const float pb = m_options.getPaddingBottom() * scale;
    return {
        { m_bounds.position.x + pl, m_bounds.position.y + pt },
        { std::max(0.f, m_bounds.size.x - pl - pr),
          std::max(0.f, m_bounds.size.y - pt - pb) }
    };
}

float Textbox::lineHeight() const {
    if (!m_fontPtr) return static_cast<float>(m_options.getCharSize());
    const float scale = m_uiloRef ? m_uiloRef->getScale() : 1.f;
    const unsigned int cs = static_cast<unsigned int>(m_options.getCharSize() * scale);
    return m_fontPtr->getLineSpacing(cs);
}

std::u32string Textbox::displayText() const {
    if (m_options.getPasswordMode() && !m_text.empty())
        return std::u32string(m_text.size(), U'*');
    return m_text;
}

sf::Vector2f Textbox::charScreenPos(size_t idx) const {
    if (!m_sfText) return m_textOrigin;
    size_t dispIdx = shouldWrap(m_options) ? textToDisplay(idx) : idx;
    dispIdx = std::min(dispIdx, [&]() -> size_t {
        // length of the string currently in m_sfText
        return m_sfText->getString().getSize();
    }());
    return glyphPos(*m_sfText, dispIdx);
}

void Textbox::rebuildSfText() {
    if (!m_fontPtr) return;
    const float scale = m_uiloRef ? m_uiloRef->getScale() : 1.f;
    const unsigned int cs = static_cast<unsigned int>(m_options.getCharSize() * scale);

    // Determine display string
    std::string displayStr;
    if (shouldWrap(m_options)) {
        rebuildWrapped();
        displayStr = m_wrappedDisplay;
        m_lastWrapWidth = textArea().size.x;
    } else {
        displayStr = u32ToUtf8(displayText());
    }

    m_sfText.emplace(*m_fontPtr, displayStr, cs);

    std::uint32_t style = 0;
    if (m_options.getBold())   style |= static_cast<uint32_t>(sf::Text::Style::Bold);
    if (m_options.getItalic()) style |= static_cast<uint32_t>(sf::Text::Style::Italic);
    m_sfText->setStyle(style);
    m_sfText->setFillColor(m_options.getTextColor());

    if (hasAlign(m_options.getTextAlignX(), Align::CenterX))
        m_sfText->setLineAlignment(sf::Text::LineAlignment::Center);
    else if (hasAlign(m_options.getTextAlignX(), Align::Right))
        m_sfText->setLineAlignment(sf::Text::LineAlignment::Right);
    else
        m_sfText->setLineAlignment(sf::Text::LineAlignment::Left);

    // Cache stable vertical bounds using caps-only reference so that all-caps
    // text centers perfectly on the visible cap-height, and descenders on
    // lowercase chars fall naturally below that centre line without shifting it.
    {
        sf::Text refText(*m_fontPtr, "A", cs);
        refText.setStyle(style);
        m_stableLineBounds = refText.getLocalBounds();
    }

    m_textDirty = false;
}

void Textbox::computeTextOrigin() {
    if (!m_sfText) return;
    const sf::FloatRect area = textArea();
    const sf::FloatRect lb   = m_sfText->getLocalBounds();
    // Stable reference bounds: always use the 'Ag' reference for vertical
    // positioning so the baseline never shifts as different characters are typed.
    const sf::FloatRect slb  = m_stableLineBounds;

    // Horizontal: use actual text bounds (grows as text is typed)
    float ox;
    if (hasAlign(m_options.getTextAlignX(), Align::CenterX))
        ox = area.position.x + area.size.x * 0.5f - m_scrollOffsetX;
    else if (hasAlign(m_options.getTextAlignX(), Align::Right))
        ox = area.position.x + area.size.x - m_scrollOffsetX;
    else
        ox = area.position.x - lb.position.x - m_scrollOffsetX;

    // Vertical: single-line boxes respect textAlignY as usual.
    // Multiline boxes always derive the first-line offset from the *initial*
    // (modifier-specified) box height so that line 1 sits at the same position
    // whether there are 1 or N lines — CenterY computed against the full auto-grown
    // area.size.y would push line 1 further down each time a new line was added.
    float oy;
    if (m_options.getMultiline()) {
        const float sc          = m_uiloRef ? m_uiloRef->getScale() : 1.f;
        const float ptS         = m_options.getPaddingTop()    * sc;
        const float pbS         = m_options.getPaddingBottom() * sc;
        const float scaledInitH = m_initialHeight * sc;  // re-scale each frame
        const float initAreaH   = m_initialHeightSet
                                ? std::max(slb.size.y, scaledInitH - ptS - pbS)
                                : slb.size.y;
        const float topOffset = (initAreaH - slb.size.y) * 0.5f;
        oy = area.position.y + topOffset - slb.position.y - m_scrollOffsetY;
    } else if (hasAlign(m_options.getTextAlignY(), Align::CenterY)) {
        oy = area.position.y + (area.size.y - slb.size.y) * 0.5f - slb.position.y - m_scrollOffsetY;
    } else if (hasAlign(m_options.getTextAlignY(), Align::Bottom)) {
        oy = area.position.y + area.size.y - slb.size.y - slb.position.y - m_scrollOffsetY;
    } else {
        oy = area.position.y - slb.position.y - m_scrollOffsetY;
    }

    m_textOrigin = { ox, oy };
    m_sfText->setPosition(m_textOrigin);
}

void Textbox::ensureCursorVisible() {
    if (!m_sfText) return;
    const sf::FloatRect area = textArea();
    const float lh           = lineHeight();

    sf::Vector2f cp = charScreenPos(m_cursorPos);

    // Horizontal scroll (used in single-line mode; also keeps cursor on-screen in multiline)
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

    // Vertical scroll (multiline)
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

size_t Textbox::hitTestChar(sf::Vector2f screenPos) const {
    if (!m_sfText) return 0;
    const float lh = std::max(1.f, lineHeight());
    const size_t dispLen = m_sfText->getString().getSize();

    // Pass 1: find which visual line the click is on (pure vertical, no x influence).
    // findCharacterPos returns the TOP of the character cell, so compare against the
    // cell centre (cp.y + lh/2) — this places the boundary exactly at the bottom
    // edge of each line rather than in the middle of the visible glyph area.
    float bestLineY    = 0.f;
    float bestLineDist = std::numeric_limits<float>::max();
    for (size_t i = 0; i <= dispLen; ++i) {
        float cy     = glyphPos(*m_sfText, i).y;
        float center = cy + lh * 0.5f;
        float dy     = std::abs(center - screenPos.y);
        if (dy < bestLineDist) { bestLineDist = dy; bestLineY = cy; }
    }

    // Pass 2: among chars on that line (y within lh/2), find the nearest x.
    size_t best     = 0;
    float bestXDist = std::numeric_limits<float>::max();
    for (size_t i = 0; i <= dispLen; ++i) {
        sf::Vector2f cp = glyphPos(*m_sfText, i);
        if (std::abs(cp.y - bestLineY) > lh * 0.5f) continue;
        float dx = std::abs(cp.x - screenPos.x);
        if (dx < bestXDist) { bestXDist = dx; best = i; }
    }

    return shouldWrap(m_options) ? displayToText(best) : best;
}

bool Textbox::hasSelection() const {
    return m_cursorPos != m_anchorPos;
}

void Textbox::deleteSelection() {
    if (!hasSelection()) return;
    const size_t lo = std::min(m_cursorPos, m_anchorPos);
    const size_t hi = std::max(m_cursorPos, m_anchorPos);
    m_text.erase(lo, hi - lo);
    m_cursorPos = m_anchorPos = lo;
    m_textDirty = true;
}

void Textbox::resetBlink() {
    m_blinkTimer    = 0.f;
    m_cursorVisible = true;
}

size_t Textbox::lineStart(size_t pos) const {
    while (pos > 0 && m_text[pos - 1] != U'\n') --pos;
    return pos;
}

size_t Textbox::lineEnd(size_t pos) const {
    while (pos < m_text.size() && m_text[pos] != U'\n') ++pos;
    return pos;
}

size_t Textbox::wordLeft(size_t pos) const {
    if (pos == 0) return 0;
    --pos;
    while (pos > 0 && !isWordChar(m_text[pos])) --pos;
    while (pos > 0 && isWordChar(m_text[pos - 1])) --pos;
    return pos;
}

size_t Textbox::wordRight(size_t pos) const {
    const size_t n = m_text.size();
    while (pos < n && isWordChar(m_text[pos])) ++pos;
    while (pos < n && !isWordChar(m_text[pos])) ++pos;
    return pos;
}

// ---------------------------------------------------------------------------
// Soft-wrap: build display string with virtual newlines
// ---------------------------------------------------------------------------

void Textbox::rebuildWrapped() {
    m_softWrapAt.clear();
    if (!m_fontPtr) { m_wrappedDisplay = u32ToUtf8(m_text); return; }

    const float scale     = m_uiloRef ? m_uiloRef->getScale() : 1.f;
    const unsigned int cs = static_cast<unsigned int>(m_options.getCharSize() * scale);
    const float maxWidth  = textArea().size.x;
    const std::u32string& src = m_text;

    if (maxWidth <= 0.f) { m_wrappedDisplay = u32ToUtf8(src); return; }

    std::uint32_t style = 0;
    if (m_options.getBold())   style |= static_cast<uint32_t>(sf::Text::Style::Bold);
    if (m_options.getItalic()) style |= static_cast<uint32_t>(sf::Text::Style::Italic);
    sf::Text probe(*m_fontPtr, "", cs);
    probe.setStyle(style);

    std::u32string result;
    result.reserve(src.size() + 8);

    // Process paragraph [pStart, pEnd) from src, word-wrapping to maxWidth.
    // Appends wrapped content to result; records m_softWrapAt src indices for
    // each soft newline inserted.
    auto processParagraph = [&](size_t pStart, size_t pEnd) {
        std::u32string lineStr;
        size_t j = pStart;

        // Break overlong content character-by-character into lineStr/result.
        auto charWrap = [&](size_t from, size_t to) {
            for (size_t k = from; k < to; ++k) {
                probe.setString(u32ToUtf8(lineStr + src[k]));
                if (!lineStr.empty() && probe.getLocalBounds().size.x > maxWidth) {
                    result += lineStr;
                    m_softWrapAt.push_back(k);
                    result += U'\n';
                    lineStr.clear();
                }
                lineStr += src[k];
            }
        };

        while (j < pEnd) {
            // Find end of word (non-space) and end of trailing spaces
            size_t wordEnd = j;
            while (wordEnd < pEnd && src[wordEnd] != U' ') ++wordEnd;
            size_t tokenEnd = wordEnd;
            while (tokenEnd < pEnd && src[tokenEnd] == U' ') ++tokenEnd;
            if (tokenEnd == j) ++tokenEnd; // stall guard

            std::u32string token(src.begin() + (std::ptrdiff_t)j,
                                 src.begin() + (std::ptrdiff_t)tokenEnd);

            // Try full token on current line
            probe.setString(u32ToUtf8(lineStr + token));
            float tw = probe.getLocalBounds().size.x;

            if (tw <= maxWidth) {
                lineStr += token;
            } else {
                // Measure just the word (no trailing spaces) on its own
                std::u32string wordOnly(src.begin() + (std::ptrdiff_t)j,
                                        src.begin() + (std::ptrdiff_t)wordEnd);
                probe.setString(u32ToUtf8(wordOnly));
                float wordW = probe.getLocalBounds().size.x;

                if (wordW <= maxWidth) {
                    // Word fits on a fresh line — flush current line first
                    if (!lineStr.empty()) {
                        result += lineStr;
                        m_softWrapAt.push_back(j);
                        result += U'\n';
                        lineStr.clear();
                    }
                    lineStr = token;
                } else {
                    // Word is wider than the available width — char-wrap it
                    if (!lineStr.empty()) {
                        result += lineStr;
                        m_softWrapAt.push_back(j);
                        result += U'\n';
                        lineStr.clear();
                    }
                    charWrap(j, wordEnd);
                    // Append trailing spaces (next iteration will handle overflow)
                    lineStr += std::u32string(src.begin() + (std::ptrdiff_t)wordEnd,
                                             src.begin() + (std::ptrdiff_t)tokenEnd);
                }
            }
            j = tokenEnd;
        }
        result += lineStr;
    };

    // Walk src; at each explicit '\n' or end-of-string, flush the current paragraph.
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

    m_wrappedDisplay = u32ToUtf8(result);
}

// textToDisplay: given an index in m_text, return the corresponding index in
// m_wrappedDisplay. A soft wrap at position p inserts a '\n' BEFORE src[p],
// so every index >= p shifts up by one.
size_t Textbox::textToDisplay(size_t textIdx) const {
    size_t extra = 0;
    for (size_t wrapPos : m_softWrapAt) {
        if (textIdx >= wrapPos) ++extra;
        else break;
    }
    return textIdx + extra;
}

// displayToText: reverse of textToDisplay.
size_t Textbox::displayToText(size_t dispIdx) const {
    size_t extra = 0;
    for (size_t wrapPos : m_softWrapAt) {
        // Display position of this wrap's '\n' is (wrapPos + extra)
        if (dispIdx > wrapPos + extra) ++extra;
        else break;
    }
    return dispIdx >= extra ? dispIdx - extra : 0;
}

// ---------------------------------------------------------------------------
// getString / setString
// ---------------------------------------------------------------------------

std::string Textbox::getString() const {
    return u32ToUtf8(m_text);
}

void Textbox::setString(const std::string& s) {
    m_text      = utf8ToU32(s);
    m_cursorPos = m_anchorPos = std::min(m_cursorPos, m_text.size());
    m_textDirty = true;
    m_scrollOffsetX = m_scrollOffsetY = 0.f;
}

// ---------------------------------------------------------------------------
// update
// ---------------------------------------------------------------------------

void Textbox::update(sf::FloatRect& parentBounds, float dt) {
    resize(parentBounds);
    if (!m_fontPtr) return;

    const float scale = m_uiloRef ? m_uiloRef->getScale() : 1.f;
    if (scale != m_lastScale) {
        m_lastScale     = scale;
        m_textDirty     = true;
        m_scrollOffsetX = m_scrollOffsetY = 0.f;
    }

    // If wrapping is active and the available width changed, force a rebuild
    const bool wrapMode = shouldWrap(m_options);
    if (!m_textDirty && wrapMode && textArea().size.x != m_lastWrapWidth)
        m_textDirty = true;

    if (m_textDirty) {
        rebuildSfText();
    } else if (m_sfText) {
        // Cheaper path: just sync the string without recreating the object
        if (wrapMode) {
            // Re-wrap if width changed (handled above) but otherwise just use cached wrapped display
            m_sfText->setString(m_wrappedDisplay);
        } else {
            m_sfText->setString(u32ToUtf8(displayText()));
        }
    }

    // Auto-height: grow one line at a time as content wraps.
    // Uses line count x lineSpacing so descender chars never affect the box height.
    if (wrapMode) {
        // Capture the originally-specified height so we never shrink below it.
        if (!m_initialHeightSet) {
            m_initialHeight    = m_bounds.size.y / scale;  // store unscaled so DPI changes don't break it
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
                               ? static_cast<float>(ml) * lh + pt + pb
                               : wantedH;
        m_bounds.size.y = std::max(m_initialHeight, clampedH);

        // Clamp scroll: when content fits, no scroll needed; when clamped, cap at overflow
        const float maxScroll = (ml > 0 && lineCount > ml)
                                ? static_cast<float>(lineCount - ml) * lh
                                : 0.f;
        m_scrollOffsetY = std::max(0.f, std::min(m_scrollOffsetY, maxScroll));

        const float unscaled = m_bounds.size.y / scale;
        if (m_modifier.getHeight().value != unscaled)
            m_modifier.setHeight(Dimension{unscaled, false});
    }

    // Drag-select: only start tracking once the mouse has moved more than 3px from
    // the initial click, so a plain click never creates a micro-selection.
    if (m_mouseDown) {
        if (!sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
            m_mouseDown = false;
            m_dragging  = false;
        } else if (m_sfText && m_uiloRef) {
            const sf::Vector2f mp = m_uiloRef->getMousePosition();
            const float dx = mp.x - m_mouseDownPos.x;
            const float dy = mp.y - m_mouseDownPos.y;
            if (!m_dragging && (dx * dx + dy * dy) > 9.f) // 3px threshold
                m_dragging = true;
            if (m_dragging) {
                const size_t idx = hitTestChar(mp);
                if (idx != m_cursorPos) {
                    m_cursorPos = idx;
                    resetBlink();
                }
            }
        }
    }

    computeTextOrigin();
    if (m_needsCursorScroll) {
        ensureCursorVisible();
        m_needsCursorScroll = false;
    }

    // Blink cursor
    if (m_focused) {
        m_blinkTimer += dt;
        const float half = m_options.getBlinkRate() * 0.5f;
        if (m_blinkTimer >= half) {
            m_blinkTimer   -= half;
            m_cursorVisible = !m_cursorVisible;
        }
    }
}

// ---------------------------------------------------------------------------
// render
// ---------------------------------------------------------------------------

void Textbox::render(sf::RenderTarget& target) {
    if (!m_fontPtr) return;

    const float scale  = m_uiloRef ? m_uiloRef->getScale() : 1.f;
    const float r      = m_options.getRounding() * scale;
    const sf::Color bg = m_options.getBackgroundColor();
    const sf::Vector2u winSize = target.getSize();
    const sf::View savedView   = target.getView();

    // Set a pixel-accurate base view for the background
    sf::View pixelView(sf::FloatRect{
        {0.f, 0.f},
        {static_cast<float>(winSize.x), static_cast<float>(winSize.y)}
    });
    target.setView(pixelView);

    // ---- Background ----
    if (bg.a > 0) {
        if (r <= 0.f) {
            sf::RectangleShape bg_rect(m_bounds.size);
            bg_rect.setPosition(m_bounds.position);
            bg_rect.setFillColor(bg);
            target.draw(bg_rect);
        } else {
            sf::ConvexShape rounded = makeRoundedRect(m_bounds.position, m_bounds.size, r);
            rounded.setFillColor(bg);
            target.draw(rounded);
        }
    }

    // ---- Focus outline ----
    const float ot = m_options.getOutlineThickness() * scale;
    if (m_focused && ot > 0.f && m_options.getOutlineColor().a > 0) {
        if (r <= 0.f) {
            sf::RectangleShape ol(m_bounds.size);
            ol.setPosition(m_bounds.position);
            ol.setFillColor(sf::Color::Transparent);
            ol.setOutlineColor(m_options.getOutlineColor());
            ol.setOutlineThickness(-ot);
            target.draw(ol);
        } else {
            sf::ConvexShape ol = makeRoundedRect(m_bounds.position, m_bounds.size, r);
            ol.setFillColor(sf::Color::Transparent);
            ol.setOutlineColor(m_options.getOutlineColor());
            ol.setOutlineThickness(-ot);
            target.draw(ol);
        }
    }

    // ---- Text-area content (clipped) ----
    const sf::FloatRect area = textArea();
    if (area.size.x > 0.f && area.size.y > 0.f && m_sfText) {
        sf::View clipView;
        clipView.setCenter(area.position + area.size / 2.f);
        clipView.setSize(area.size);
        clipView.setViewport(sf::FloatRect{
            { area.position.x / static_cast<float>(winSize.x),
              area.position.y / static_cast<float>(winSize.y) },
            { area.size.x     / static_cast<float>(winSize.x),
              area.size.y     / static_cast<float>(winSize.y) }
        });
        target.setView(clipView);

        const float lh = lineHeight();

        // -- Selection highlight --
        if (hasSelection()) {
            const size_t selMin = std::min(m_cursorPos, m_anchorPos);
            const size_t selMax = std::max(m_cursorPos, m_anchorPos);
            const sf::Color selCol = m_options.getSelectionColor();

            for (size_t i = selMin; i < selMax; ++i) {
                const bool wm = shouldWrap(m_options);
                sf::Vector2f p0 = glyphPos(*m_sfText, wm ? textToDisplay(i)     : i);
                sf::Vector2f p1 = glyphPos(*m_sfText, wm ? textToDisplay(i + 1) : i + 1);

                float w;
                if (p1.y > p0.y + lh * 0.5f) {
                    // Newline char — fill to right edge
                    w = area.position.x + area.size.x - p0.x;
                } else {
                    w = p1.x - p0.x;
                }
                if (w > 0.f) {
                    sf::RectangleShape sel({ w, lh });
                    sel.setPosition(p0);
                    sel.setFillColor(selCol);
                    target.draw(sel);
                }
            }
        }

        // -- Placeholder or text --
        if (m_text.empty() && !m_options.getPlaceholder().empty()) {
            const unsigned int cs = static_cast<unsigned int>(m_options.getCharSize() * scale);
            sf::Text ph(*m_fontPtr, m_options.getPlaceholder(), cs);
            ph.setFillColor(m_options.getPlaceholderColor());
            ph.setLineAlignment(m_sfText->getLineAlignment());
            ph.setPosition(m_textOrigin);
            target.draw(ph);
        } else {
            target.draw(*m_sfText);
        }

        // -- Blinking cursor --
        if (m_focused && m_cursorVisible) {
            sf::Vector2f cp = charScreenPos(m_cursorPos);
            const float cw  = std::max(1.f, m_options.getCursorWidth() * scale);
            sf::RectangleShape cursor({ cw, lh });
            cursor.setPosition({ cp.x - cw * 0.5f, cp.y });
            cursor.setFillColor(m_options.getCursorColor());
            target.draw(cursor);
        }
    }

    target.setView(savedView);
}

// ---------------------------------------------------------------------------
// Input: click / hover / deactivate
// ---------------------------------------------------------------------------

bool Textbox::checkLeftClick(const sf::Vector2f& mousePos) {
    if (!m_bounds.contains(mousePos)) return false;
    m_uiloRef->setCurrInteractible(this);
    m_focused    = true;
    m_mouseDown     = true;
    m_dragging      = false;
    m_mouseDownPos  = mousePos;
    resetBlink();

    if (m_sfText) {
        const bool shiftHeld = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift) ||
                               sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RShift);
        const size_t idx = hitTestChar(mousePos);
        m_cursorPos  = idx;
        if (!shiftHeld) m_anchorPos = idx;
        m_preferredX = charScreenPos(idx).x;
    }
    return true;
}

bool Textbox::checkHover(const sf::Vector2f& mousePos) {
    if (m_bounds.contains(mousePos) && m_uiloRef)
        m_uiloRef->requestCursor(sf::Cursor::Type::Text, 1);
    return Element::checkHover(mousePos);
}

void Textbox::onDeactivate() {
    m_focused       = false;
    m_cursorVisible = false;
    m_mouseDown     = false;
    m_dragging      = false;
}

// ---------------------------------------------------------------------------
// Input: scroll wheel
// ---------------------------------------------------------------------------

bool Textbox::checkScroll(const sf::Vector2f& mousePos, float delta) {
    if (!m_bounds.contains(mousePos)) return false;
    if (!m_options.getMultiline()) return false;

    const int ml = m_options.getMaxResizeLines();
    if (ml <= 0) return false; // not in scroll mode — let parent container scroll

    int lineCount = 1;
    for (char c : m_wrappedDisplay) if (c == '\n') ++lineCount;
    if (lineCount <= ml) return false; // content fits — propagate to parent

    const float lh        = lineHeight();
    const float maxScroll = static_cast<float>(lineCount - ml) * lh;
    // Snap to whole-line increments: round current offset to nearest line, then
    // step by the number of lines the scroll event represents (delta is in "ticks").
    const float currentLine = std::round(m_scrollOffsetY / lh);
    const float targetLine  = currentLine - delta;   // delta>0 = scroll up (reveal top)
    m_scrollOffsetY = std::max(0.f, std::min(targetLine * lh, maxScroll));
    computeTextOrigin();
    return true;
}

// ---------------------------------------------------------------------------
// Input: text / key events
// ---------------------------------------------------------------------------

void Textbox::handleTextInput(char32_t c) {
    if (!m_focused) return;
    // Filter control characters — Enter, Backspace, Delete, Escape, etc.
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

void Textbox::handleKeyInput(sf::Keyboard::Key key, bool shift, bool ctrl) {
    if (!m_focused) return;

    using Key = sf::Keyboard::Key;
    const size_t n = m_text.size();

    // Move cursor, optionally extending selection (shift) or collapsing it
    auto moveCursor = [&](size_t newPos) {
        m_cursorPos = newPos;
        if (!shift) m_anchorPos = newPos;
        resetBlink();
    };

    switch (key) {

        // ---- Navigation ----
        case Key::Left:
            if (!shift && hasSelection()) {
                moveCursor(std::min(m_cursorPos, m_anchorPos));
            } else if (m_cursorPos > 0) {
                moveCursor(ctrl ? wordLeft(m_cursorPos) : m_cursorPos - 1);
            }
            m_preferredX = charScreenPos(m_cursorPos).x;
            break;

        case Key::Right:
            if (!shift && hasSelection()) {
                moveCursor(std::max(m_cursorPos, m_anchorPos));
            } else if (m_cursorPos < n) {
                moveCursor(ctrl ? wordRight(m_cursorPos) : m_cursorPos + 1);
            }
            m_preferredX = charScreenPos(m_cursorPos).x;
            break;

        case Key::Up:
            if (m_options.getMultiline() && m_sfText) {
                sf::Vector2f cp     = charScreenPos(m_cursorPos);
                float targetY       = cp.y - lineHeight();
                sf::Vector2f target = { m_preferredX, targetY + lineHeight() * 0.5f };
                moveCursor(hitTestChar(target));
            }
            break;

        case Key::Down:
            if (m_options.getMultiline() && m_sfText) {
                sf::Vector2f cp     = charScreenPos(m_cursorPos);
                float targetY       = cp.y + lineHeight();
                sf::Vector2f target = { m_preferredX, targetY + lineHeight() * 0.5f };
                moveCursor(hitTestChar(target));
            }
            break;

        case Key::Home:
            moveCursor(ctrl ? 0 : lineStart(m_cursorPos));
            m_preferredX = charScreenPos(m_cursorPos).x;
            break;

        case Key::End:
            moveCursor(ctrl ? n : lineEnd(m_cursorPos));
            m_preferredX = charScreenPos(m_cursorPos).x;
            break;

        // ---- Editing ----
        case Key::Backspace:
            if (hasSelection()) {
                deleteSelection();
            } else if (m_cursorPos > 0) {
                size_t newPos = ctrl ? wordLeft(m_cursorPos) : m_cursorPos - 1;
                m_text.erase(newPos, m_cursorPos - newPos);
                m_cursorPos = m_anchorPos = newPos;
                m_textDirty = true;
            }
            resetBlink();
            if (m_options.getOnStringChanged()) m_options.getOnStringChanged()(getString());
            break;

        case Key::Delete:
            if (hasSelection()) {
                deleteSelection();
            } else if (m_cursorPos < n) {
                size_t endPos = ctrl ? wordRight(m_cursorPos) : m_cursorPos + 1;
                m_text.erase(m_cursorPos, endPos - m_cursorPos);
                m_textDirty = true;
            }
            resetBlink();
            if (m_options.getOnStringChanged()) m_options.getOnStringChanged()(getString());
            break;

        case Key::Enter:
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

        case Key::Escape:
            if (m_uiloRef) m_uiloRef->setCurrInteractible(nullptr);
            return;  // don't scroll to cursor on Escape
            break;

        // ---- Ctrl shortcuts ----
        case Key::A:
            if (ctrl) { m_anchorPos = 0; m_cursorPos = n; resetBlink(); }
            break;

        case Key::C:
            if (ctrl && hasSelection()) {
                const size_t lo = std::min(m_cursorPos, m_anchorPos);
                const size_t hi = std::max(m_cursorPos, m_anchorPos);
                sf::Clipboard::setString(sf::String(u32ToUtf8(m_text.substr(lo, hi - lo))));
            }
            break;

        case Key::X:
            if (ctrl && hasSelection()) {
                const size_t lo = std::min(m_cursorPos, m_anchorPos);
                const size_t hi = std::max(m_cursorPos, m_anchorPos);
                sf::Clipboard::setString(sf::String(u32ToUtf8(m_text.substr(lo, hi - lo))));
                deleteSelection();
                if (m_options.getOnStringChanged()) m_options.getOnStringChanged()(getString());
            }
            break;

        case Key::V:
            if (ctrl) {
                sf::String sfStr = sf::Clipboard::getString();
                std::u32string pasted(sfStr.begin(), sfStr.end());
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
            break;

        default:
            break;
    }
    m_needsCursorScroll = true;
}

} // namespace uilo
