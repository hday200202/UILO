#include "Text.hpp"
#include "../../UILO.hpp"
#include "../../utils/Alignment.hpp"
#include "../../utils/Resources.hpp"
#include "../../renderer/Renderer.hpp"

#include <sstream>

namespace uilo {

/*
    Text(Modifier modifier, TextOptions options, const std::string& name):
    - Params:   Modifier modifier, TextOptions options, const std::string& name
    - Returns:  Text
    - Desc:     Constructs a text element from a modifier and its options,
                taking the initial string and character size from them. The font
                is not loaded here: there is no renderer until the element is
                bound to a UILO, so that waits for the first update.
*/
Text::Text(
    Modifier modifier,
    TextOptions options,
    const std::string& name
) : m_options(options),
    m_content(options.getContent()),
    m_charSize(options.hasCharSize() ? options.getCharSize() : 30) {
    m_modifier = modifier;
    m_name     = name;
    m_type     = ElementType::Text;
}


/*
    init():
    - Params:   none
    - Returns:  void
    - Desc:     Loads the font, once, on the first update that has a UILO to
                load it through. A registered font name --
                Resources::fonts::default_, or anything the application added --
                resolves to its path here, while a plain path is handed back
                unchanged, so setFont("assets/fonts/X.ttf") is unaffected. A
                failed load leaves the element unloaded, which makes it draw
                nothing rather than crash, and the next update tries again.
*/
void Text::init() {
    if (m_loaded || !m_uiloRef) return;

    const std::string_view resolved =
        Resources::get().fontRegistry().resolve(m_options.getFontPath());

    Font f = m_uiloRef->getRenderer().loadFont(std::string(resolved));
    if (f.valid()) {
        m_fontId = f.id;
        m_loaded = true;
        m_wrappedContent = m_content;
    }
}


/*
    wrapContent(float maxWidth):
    - Params:   float maxWidth
    - Returns:  std::string -- the string with newlines inserted
    - Desc:     Greedy word wrap at a pixel width. Words are added to the
                current line until measuring one more would overflow, at which
                point the line is broken. A single word wider than maxWidth is
                left on its own line rather than split, since breaking mid-word
                reads worse than overflowing. Returns the string unchanged when
                there is no font loaded or no width to wrap against.
*/
std::string Text::wrapContent(float maxWidth) const {
    if (!m_uiloRef || !m_loaded || maxWidth <= 0.f) return m_content;

    auto& renderer = m_uiloRef->getRenderer();
    Font f; f.id = m_fontId;
    const float scale = m_uiloRef->getScale();
    const float pxH   = (float)m_charSize * scale;

    std::string result;
    result.reserve(m_content.size() + 16);

    std::stringstream ss(m_content);
    std::string word;
    std::string line;
    while (std::getline(ss, word, ' ')) {
        std::string candidate = line.empty() ? word : (line + " " + word);
        TextMetrics m = renderer.measureText(candidate, f, pxH);
        if (m.size.x > maxWidth && !line.empty()) {
            result += line; result += '\n';
            line = word;
        } else {
            line = candidate;
        }
    }
    result += line;
    return result;
}


/*
    rebuildText():
    - Params:   none
    - Returns:  void
    - Desc:     Re-derives the string that is actually drawn, re-wrapping it
                when wrapping is on and a width is known, and invalidates the
                cached layout metrics so the next draw measures afresh.
*/
void Text::rebuildText() {
    if (m_options.getWrap() && m_lastWrapWidth > 0.f) {
        m_wrappedContent = wrapContent(m_lastWrapWidth);
    } else {
        m_wrappedContent = m_content;
    }
    m_cachedMetricsValid = false;
}


/*
    isLoaded():
    - Params:   none
    - Returns:  bool
    - Desc:     Whether the font loaded. False until the first update after the
                element is bound to a UILO, and permanently false when the font
                could not be read.
*/
bool Text::isLoaded() const { return m_loaded; }


/*
    setString(const std::string& content):
    - Params:   const std::string& content
    - Returns:  void
    - Desc:     Replaces the string being drawn. Setting the same string is a
                no-op, so calling this every frame from a handler does not force
                a redraw or a re-measure. Re-wraps immediately when the font is
                loaded; otherwise the first update does it.
*/
void Text::setString(const std::string& content) {
    if (content == m_content) return;

    m_content = content;
    m_wrappedContent = content;
    m_dirty = true;
    m_cachedMetricsValid = false;
    if (m_loaded) rebuildText();
}


/*
    update(Rectf& parentBounds, float dt):
    - Params:   Rectf& parentBounds, float dt
    - Returns:  void
    - Desc:     Loads the font if that has not happened yet, resolves the
                element's bounds, and re-derives the drawn string when anything
                it depends on has moved. With no explicit character size the
                glyphs are sized from the element's own height, so a Text in a
                row grows with it; a wrapping Text re-flows when its width
                changes; and any change of UI scale invalidates the measurements
                either way. Returns early while the font is still unloaded,
                which is what makes a missing font draw nothing instead of
                crashing.
*/
void Text::update(Rectf& parentBounds, float dt) {
    (void)dt;
    if (!m_loaded) init();
    if (!m_loaded) return;

    resize(parentBounds);

    float scale = m_uiloRef ? m_uiloRef->getScale() : 1.f;
    bool needRebuild = false;

    /* No explicit char size, so the height drives it. */
    if (!m_options.hasCharSize()) {
        const unsigned int autoCs = std::max(1u,
            static_cast<unsigned int>(contentArea().size.y * 0.6f / scale));
        if (autoCs != m_charSize) {
            m_charSize   = autoCs;
            needRebuild  = true;
        }
    }

    if (m_options.getWrap() && contentArea().size.x != m_lastWrapWidth) {
        m_lastWrapWidth = contentArea().size.x;
        needRebuild = true;
    }
    if (scale != m_lastScale) {
        m_lastScale = scale;
        needRebuild = true;
    }
    if (needRebuild) rebuildText();
}


/*
    render():
    - Params:   none
    - Returns:  void
    - Desc:     Draws the string, placed inside the element's bounds according
                to the options' text alignment. Metrics are measured only when
                the cache is stale, so an unchanged string costs no UTF-8 or
                glyph-table walk per frame. The colour resolves through the
                Palette, falling back to the literal when there is no UILO.
*/
void Text::render() {
    if (!m_modifier.getVisible()) { m_dirty = false; return; }
    m_dirty = false;
    if (!m_loaded || !m_uiloRef) return;
    if (m_wrappedContent.empty()) return;

    auto& renderer = m_uiloRef->getRenderer();
    Font f; f.id = m_fontId;
    const float scale = m_uiloRef->getScale();
    const float pxH   = (float)m_charSize * scale;

    if (!m_cachedMetricsValid) {
        m_cachedMetrics      = renderer.measureText(m_wrappedContent, f, pxH);
        m_cachedMetricsValid = true;
    }
    const TextMetrics& m = m_cachedMetrics;

    /* Place the measured block inside the element's own bounds. */
    /* Laid out inside the inner padding rather than against the raw bounds. */
    const Rectf area = contentArea();
    Vec2f pos = area.position;
    switch (m_options.getTextAlignX()) {
        case Align::CenterX: pos.x += (area.size.x - m.size.x) * 0.5f; break;
        case Align::Right:   pos.x += (area.size.x - m.size.x);        break;
        default: break;
    }
    switch (m_options.getTextAlignY()) {
        case Align::CenterY: pos.y += (area.size.y - m.size.y) * 0.5f; break;
        case Align::Bottom:  pos.y += (area.size.y - m.size.y);        break;
        default: break;
    }

    const Color textColor = m_uiloRef
        ? m_uiloRef->getPalette().resolve(m_options.getColorRole(), m_options.getColor())
        : m_options.getColor();
    renderer.drawText(m_wrappedContent, pos, f, pxH, textColor,
                      TextStyle{m_options.getBold(), m_options.getItalic()});
}

} // namespace uilo
