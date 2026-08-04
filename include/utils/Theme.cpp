#include "Theme.hpp"

namespace uilo {

/*
    current():
    - Params:   none
    - Returns:  Theme&
    - Desc:     The process-wide theme. A function-local static rather than a
                namespace-scope one so it is constructed on first use, which
                makes it safe to touch from a static initializer.
*/
Theme& Theme::current() {
    static Theme instance;
    return instance;
}


/*
    set(const Theme& theme):
    - Params:   const Theme& theme
    - Returns:  void
    - Desc:     Replaces the active theme outright, dropping anything the
                previous one said.
*/
void Theme::set(const Theme& theme) {
    current() = theme;
}


/*
    reset():
    - Params:   none
    - Returns:  void
    - Desc:     Clears every themed property, so every type goes back to its own
                default.
*/
void Theme::reset() {
    current() = Theme{};
}


/*
    resolveRounding(const std::optional<float>& own, float fallback):
    - Params:   const std::optional<float>& own, float fallback
    - Returns:  float
    - Desc:     The element's own radius, the theme's, or the type's default, in
                that order.
*/
float Theme::resolveRounding(const std::optional<float>& own, float fallback) {
    if (own) return *own;
    if (const auto& themed = current().roundingOpt()) return *themed;
    return fallback;
}


/*
    resolveOuterPadding(const std::optional<float>& own, float fallback):
    - Params:   const std::optional<float>& own, float fallback
    - Returns:  float
    - Desc:     As resolveRounding(), for an element's inset.
*/
float Theme::resolveOuterPadding(const std::optional<float>& own, float fallback) {
    if (own) return *own;
    if (const auto& themed = current().outerPaddingOpt()) return *themed;
    return fallback;
}


/*
    resolveInnerPadding(const std::optional<float>& own, float fallback):
    - Params:   const std::optional<float>& own, float fallback
    - Returns:  float
    - Desc:     As resolveOuterPadding(), for the gap between an element's edge
                and the content it holds.
*/
float Theme::resolveInnerPadding(const std::optional<float>& own, float fallback) {
    if (own) return *own;
    if (const auto& themed = current().innerPaddingOpt()) return *themed;
    return fallback;
}


/*
    resolveCharSize(const std::optional<unsigned int>& own, unsigned int fallback):
    - Params:   const std::optional<unsigned int>& own, unsigned int fallback
    - Returns:  unsigned int
    - Desc:     As resolveRounding(), for text size.
*/
unsigned int Theme::resolveCharSize(const std::optional<unsigned int>& own,
                                    unsigned int fallback) {
    if (own) return *own;
    if (const auto& themed = current().charSizeOpt()) return *themed;
    return fallback;
}


/*
    resolveIconStrokeWidth(const std::optional<float>& own, float fallback):
    - Params:   const std::optional<float>& own, float fallback
    - Returns:  float
    - Desc:     As resolveRounding(), for icon stroke weight.
*/
float Theme::resolveIconStrokeWidth(const std::optional<float>& own, float fallback) {
    if (own) return *own;
    if (const auto& themed = current().iconStrokeOpt()) return *themed;
    return fallback;
}


/*
    resolveFont(const std::string& own):
    - Params:   const std::string& own
    - Returns:  const std::string&
    - Desc:     The element's own font path when it has one, otherwise the
                theme's. An empty result means the embedded default face, which
                is what an unset font has always meant.
*/
const std::string& Theme::resolveFont(const std::string& own) {
    if (!own.empty()) return own;
    if (const auto& themed = current().fontOpt()) return *themed;
    return own;   /* empty: the renderer's default face */
}


/*
    hasPalette():
    - Params:   none
    - Returns:  bool
    - Desc:     Whether the theme carries a palette. UILO consults this when it
                has not been given one of its own.
*/
bool Theme::hasPalette() {
    return current().paletteOpt().has_value();
}


/*
    palette():
    - Params:   none
    - Returns:  const Palette&
    - Desc:     The themed palette, or an empty one when none is set. Never
                dangles: the empty fallback is a static, so this is always safe
                to resolve roles against (they simply fall through to the
                literal colors).
*/
const Palette& Theme::palette() {
    static const Palette empty;
    if (const auto& themed = current().paletteOpt()) return *themed;
    return empty;
}

} // namespace uilo
