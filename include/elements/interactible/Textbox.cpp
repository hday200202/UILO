#include "Textbox.hpp"
#include "../../UILO.hpp"
#include "../../renderer/Renderer.hpp"

#include <SDL3/SDL.h>
#include <algorithm>
#include <cmath>
#include <limits>

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
}

// ---------------------------------------------------------------------------
// Geometry helpers
// ---------------------------------------------------------------------------

Rectf Textbox::textArea() const {
    const float scale = m_uiloRef ? m_uiloRef->getScale() : 1.f;
    const float pl = m_options.getPaddingLeft()   * scale;
    const float pr = m_options.getPaddingRight()  * scale;
    const float pt = m_options.getPaddingTop()    * scale;
    const float pb = m_options.getPaddingBottom() * scale;
    return Rectf{
        { m_bounds.position.x + pl, m_bounds.position.y + pt },
        { std::max(0.f, m_bounds.size.x - pl - pr),
          std::max(0.f, m_bounds.size.y - pt - pb) }
    };
}

float Textbox::lineHeight() const {
    if (m_lineHeightCache > 0.f) return m_lineHeightCache;
    const float scale = m_uiloRef ? m_uiloRef->getScale() : 1.f;
    const unsigned int cs = m_options.hasCharSize() ? m_options.getCharSize()
                                                     : std::max(1u, m_autoCharSize);
    return static_cast<float>(cs) * scale * 1.2f;
}

std::u32string Textbox::displayText() const {
    if (m_options.getPasswordMode() && !m_text.empty())
        return std::u32string(m_text.size(), U'*');
    return m_text;
}

Vec2f Textbox::charScreenPos(size_t idx) const {
    if (m_charPositions.empty()) return m_textOrigin;
    const size_t dispIdx = shouldWrap(m_options) ? textToDisplay(idx) : idx;
    const size_t clamped = std::min(dispIdx, m_charPositions.size() - 1);
    const Vec2f rel = m_charPositions[clamped];
    return { m_textOrigin.x + rel.x, m_textOrigin.y + rel.y };
}

size_t Textbox::hitTestChar(Vec2f screenPos) const {
    if (m_charPositions.empty() || !m_uiloRef) return 0;
    const float lh      = std::max(1.f, lineHeight());
    const size_t dispN  = m_charPositions.size();

    // Pass 1: find which visual line the click is on
    float bestLineY    = m_textOrigin.y;
    float bestLineDist = std::numeric_limits<float>::max();
    for (size_t i = 0; i < dispN; ++i) {
        const float cy = m_textOrigin.y + m_charPositions[i].y;
        const float center = cy + lh * 0.5f;
        const float dy = std::abs(center - screenPos.y);
        if (dy < bestLineDist) { bestLineDist = dy; bestLineY = cy; }
    }

    // Pass 2: nearest x on that line
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

// ---------------------------------------------------------------------------
// Rebuilds — require a live renderer & font
// ---------------------------------------------------------------------------

void Textbox::rebuildSfText() {
    m_textDirty = false;
    m_charPositions.clear();
    m_charPositions.push_back({0.f, 0.f});
    m_displayU32.clear();
    m_wrappedDisplay.clear();

    if (!m_uiloRef) return;
    auto& renderer = m_uiloRef->getRenderer();
    if (m_options.getFontPath().empty()) return;
    Font font = renderer.loadFont(m_options.getFontPath());
    if (!font.valid()) return;

    const float scale = m_uiloRef->getScale();
    const unsigned int cs = m_options.hasCharSize() ? m_options.getCharSize()
                                                     : std::max(1u, m_autoCharSize);
    const float pxH = static_cast<float>(cs) * scale;

    // Cache real line height from the font.
    {
        TextMetrics ref = renderer.measureText("A", font, pxH);
        m_lineHeightCache = ref.lineHeight();
    }

    // Build display (UTF-32) — wrapped or raw.
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

// Build m_displayU32 (with soft '\n's) and m_softWrapAt from m_text.
void Textbox::rebuildWrapped() {
    m_softWrapAt.clear();
    m_displayU32.clear();
    if (!m_uiloRef) { m_displayU32 = m_text; m_wrappedDisplay = u32ToUtf8(m_displayU32); return; }
    auto& renderer = m_uiloRef->getRenderer();
    if (m_options.getFontPath().empty()) {
        m_displayU32 = m_text;
        m_wrappedDisplay = u32ToUtf8(m_displayU32);
        return;
    }
    Font font = renderer.loadFont(m_options.getFontPath());
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
            if (tokenEnd == j) ++tokenEnd; // stall guard

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

void Textbox::computeTextOrigin() {
    if (m_charPositions.empty()) { m_textOrigin = textArea().position; return; }
    const Rectf area = textArea();
    const float lh   = lineHeight();

    // Total height = (lines) * lh (excluding gap on the last line, but lh already
    // includes lineGap which is fine for centering)
    // Total width  = max x across all character positions (approx text bbox width)
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

    // Vertical: multiline always top-anchors (auto-grow); single-line respects align.
    float oy;
    if (m_options.getMultiline()) {
        const float sc          = m_uiloRef ? m_uiloRef->getScale() : 1.f;
        const float ptS         = m_options.getPaddingTop()    * sc;
        const float pbS         = m_options.getPaddingBottom() * sc;
        const float scaledInitH = m_initialHeight * sc;
        const float initAreaH   = m_initialHeightSet
                                ? std::max(lh, scaledInitH - ptS - pbS)
                                : lh;
        const float topOffset = (initAreaH - lh) * 0.5f;
        oy = area.position.y + topOffset - m_scrollOffsetY;
    } else if (hasAlign(m_options.getTextAlignY(), Align::CenterY)) {
        oy = area.position.y + (area.size.y - totalH) * 0.5f - m_scrollOffsetY;
    } else if (hasAlign(m_options.getTextAlignY(), Align::Bottom)) {
        oy = area.position.y + (area.size.y - totalH) - m_scrollOffsetY;
    } else {
        oy = area.position.y - m_scrollOffsetY;
    }

    m_textOrigin = { std::round(ox), std::round(oy) };
}

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

size_t Textbox::textToDisplay(size_t textIdx) const {
    size_t extra = 0;
    for (size_t wrapPos : m_softWrapAt) {
        if (textIdx >= wrapPos) ++extra;
        else break;
    }
    return textIdx + extra;
}

size_t Textbox::displayToText(size_t dispIdx) const {
    size_t extra = 0;
    for (size_t wrapPos : m_softWrapAt) {
        if (dispIdx > wrapPos + extra) ++extra;
        else break;
    }
    return dispIdx >= extra ? dispIdx - extra : 0;
}

// ---------------------------------------------------------------------------
// Selection / cursor helpers
// ---------------------------------------------------------------------------

bool Textbox::hasSelection() const {
    return m_cursorPos != m_anchorPos;
}

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

void Textbox::resetBlink() {
    m_blinkTimer    = 0.f;
    m_cursorVisible = true;
    m_dirty = true;
}

size_t Textbox::lineStart(size_t pos) const {
    if (pos == 0) return 0;
    size_t i = pos;
    while (i > 0 && m_text[i - 1] != U'\n') --i;
    return i;
}

size_t Textbox::lineEnd(size_t pos) const {
    size_t i = pos;
    while (i < m_text.size() && m_text[i] != U'\n') ++i;
    return i;
}

size_t Textbox::wordLeft(size_t pos) const {
    if (pos == 0) return 0;
    size_t i = pos - 1;
    while (i > 0 && !isWordChar(m_text[i])) --i;
    while (i > 0 && isWordChar(m_text[i - 1])) --i;
    return i;
}

size_t Textbox::wordRight(size_t pos) const {
    size_t i = pos;
    const size_t n = m_text.size();
    while (i < n && !isWordChar(m_text[i])) ++i;
    while (i < n && isWordChar(m_text[i])) ++i;
    return i;
}

// ---------------------------------------------------------------------------
// Public string API
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

    // Re-wrap if the available text width has changed.
    if (shouldWrap(m_options)) {
        const float wrapW = textArea().size.x;
        if (std::abs(wrapW - m_lastWrapWidth) > 0.5f) m_textDirty = true;
    }

    if (m_textDirty) {
        m_dirty = true;
        rebuildSfText();
    }

    // Auto-height for multiline+wrap
    if (m_options.getMultiline() && m_options.getWrap()) {
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

        // Clamp vertical scroll so empty lines never appear below the text
        // after the user deletes content.
        const float maxScroll = (ml > 0 && lineCount > ml)
                                ? static_cast<float>(lineCount - ml) * lh
                                : 0.f;
        m_scrollOffsetY = std::max(0.f, std::min(m_scrollOffsetY, maxScroll));

        const float unscaled = m_bounds.size.y / scale;
        if (m_modifier.getHeight().value != unscaled)
            m_modifier.setHeight(Dimension{unscaled, false});
    }

    // Drag-select
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

// ---------------------------------------------------------------------------
// render
// ---------------------------------------------------------------------------

void Textbox::render() {
    m_dirty = false;
    if (!m_uiloRef) return;
    auto& renderer = m_uiloRef->getRenderer();

    // Background + optional outline
    const float bgScale = m_uiloRef ? m_uiloRef->getScale() : 1.f;
    const float rounding = m_options.getRounding() * bgScale;
    Color bg = m_options.getBackgroundColor();
    Color outline = m_focused ? m_options.getOutlineColor() : Color{0,0,0,0};
    float outlineT = m_focused ? m_options.getOutlineThickness() * bgScale : 0.f;
    if (rounding > 0.f) {
        renderer.draw(RoundedRect{
            m_bounds.position, m_bounds.size, rounding, 8,
            bg, outline, outlineT});
    } else {
        renderer.draw(Rect{m_bounds.position, m_bounds.size, bg, outline, outlineT});
    }

    if (m_options.getFontPath().empty()) return;
    Font font = renderer.loadFont(m_options.getFontPath());
    if (!font.valid()) return;

    const float scale = m_uiloRef->getScale();
    const unsigned int cs = m_options.hasCharSize() ? m_options.getCharSize()
                                                    : std::max(1u, m_autoCharSize);
    const float pxH = (float)cs * scale;

    const Rectf area = textArea();
    renderer.pushScissor(area);

    const bool showPlaceholder = m_text.empty() && !m_focused;
    Color textColor = showPlaceholder ? m_options.getPlaceholderColor()
                                       : m_options.getTextColor();

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
                              font, pxH, textColor);
        }
    } else {
        // Selection rects (per character; newline fills to area right edge).
        const float lh = lineHeight();
        if (m_focused && hasSelection()) {
            const size_t lo = std::min(m_cursorPos, m_anchorPos);
            const size_t hi = std::max(m_cursorPos, m_anchorPos);
            const Color  selCol = m_options.getSelectionColor();
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
            renderer.drawText(m_wrappedDisplay, m_textOrigin, font, pxH, textColor);
        }

        // Caret
        if (m_focused && m_cursorVisible) {
            Vec2f cp = charScreenPos(m_cursorPos);
            renderer.draw(Line{
                { cp.x, cp.y },
                { cp.x, cp.y + lh },
                m_options.getCursorWidth() * scale,
                m_options.getCursorColor() });
        }
    }

    renderer.popScissor();
}

// ---------------------------------------------------------------------------
// Input events
// ---------------------------------------------------------------------------

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

bool Textbox::checkHover(const Vec2f& mousePos) {
    if (m_bounds.contains(mousePos) && m_uiloRef)
        m_uiloRef->requestCursor(CursorType::Text, 1);
    return Element::checkHover(mousePos);
}

void Textbox::onDeactivate() {
    m_focused       = false;
    m_cursorVisible = false;
    m_mouseDown     = false;
    m_dragging      = false;
    m_dirty = true;
}

bool Textbox::checkScroll(const Vec2f& mousePos, float delta) {
    if (!m_bounds.contains(mousePos)) return false;
    if (!m_options.getMultiline()) return false;
    const int ml = m_options.getMaxResizeLines();
    if (ml <= 0) return false;
    int lineCount = 1;
    for (char c : m_wrappedDisplay) if (c == '\n') ++lineCount;
    if (lineCount <= ml) return false;
    const float lh        = lineHeight();
    const float maxScroll = static_cast<float>(lineCount - ml) * lh;
    const float currentLine = std::round(m_scrollOffsetY / lh);
    m_scrollOffsetY = std::max(0.f, std::min((currentLine - delta) * lh, maxScroll));
    m_dirty = true;
    return true;
}

// ---------------------------------------------------------------------------
// Text/key input
// ---------------------------------------------------------------------------

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

void Textbox::handleKeyInput(SDL_Keycode key, bool shift, bool ctrl) {
    if (!m_focused) return;

    const size_t n = m_text.size();

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
            if (ctrl) { m_anchorPos = 0; m_cursorPos = n; resetBlink(); }
            break;
        case SDLK_C:
            if (ctrl && hasSelection()) {
                const size_t lo = std::min(m_cursorPos, m_anchorPos);
                const size_t hi = std::max(m_cursorPos, m_anchorPos);
                SDL_SetClipboardText(u32ToUtf8(m_text.substr(lo, hi - lo)).c_str());
            }
            break;
        case SDLK_X:
            if (ctrl && hasSelection()) {
                const size_t lo = std::min(m_cursorPos, m_anchorPos);
                const size_t hi = std::max(m_cursorPos, m_anchorPos);
                SDL_SetClipboardText(u32ToUtf8(m_text.substr(lo, hi - lo)).c_str());
                deleteSelection();
                if (m_options.getOnStringChanged()) m_options.getOnStringChanged()(getString());
            }
            break;
        case SDLK_V:
            if (ctrl) {
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
