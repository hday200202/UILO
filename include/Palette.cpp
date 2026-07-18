#include "Palette.hpp"

namespace uilo {

namespace {
constexpr int kMaxAliasDepth = 8;
}


/*
    set(const std::string& role, Color color):
    - Params:   const std::string& role, Color color
    - Returns:  void
    - Desc:     Assigns a direct color to a role, overwriting any existing
                entry including an alias.
*/
void Palette::set(const std::string& role, Color color) {
    auto& e = m_entries[role];
    e.color   = color;
    e.aliasOf.clear();
    e.isAlias = false;
}


/*
    setAlias(const std::string& role, const std::string& target):
    - Params:   const std::string& role, const std::string& target
    - Returns:  void
    - Desc:     Makes `role` resolve to whatever `target` resolves to,
                overwriting any existing entry. Cycles are tolerated at
                resolution time, returning the fallback after a depth cap.
*/
void Palette::setAlias(const std::string& role, const std::string& target) {
    auto& e = m_entries[role];
    e.aliasOf = target;
    e.isAlias = true;
}


/*
    has(std::string_view role):
    - Params:   std::string_view role
    - Returns:  bool
    - Desc:     True when the role exists in the map as a color or alias. Does
                not follow aliases to verify they ultimately resolve.
*/
bool Palette::has(std::string_view role) const {
    return m_entries.find(role) != m_entries.end();
}


/*
    get(std::string_view role):
    - Params:   std::string_view role
    - Returns:  Color
    - Desc:     Returns the color for a role, walking alias chains. Returns the
                fallback color when unresolved.
*/
Color Palette::get(std::string_view role) const {
    return resolveImpl(role, 0);
}


/*
    resolve(std::string_view role, Color literal):
    - Params:   std::string_view role, Color literal
    - Returns:  Color
    - Desc:     Render-path convenience. Returns `literal` when the role is
                empty or "none", otherwise the palette resolution, which itself
                falls back to the palette's fallback color if the alias chain
                dies.
*/
Color Palette::resolve(std::string_view role, Color literal) const {
    if (role.empty() || role == "none") return literal;
    auto it = m_entries.find(role);
    if (it == m_entries.end()) return literal;
    if (!it->second.isAlias) return it->second.color;
    return resolveImpl(it->second.aliasOf, 1);
}


/*
    resolveImpl(std::string_view role, int depth):
    - Params:   std::string_view role, int depth
    - Returns:  Color
    - Desc:     Recursively resolves a role, following alias chains until it
                reaches a direct color. Returns the fallback color when the
                role is missing or the alias depth cap is reached, which breaks
                cycles.
*/
Color Palette::resolveImpl(std::string_view role, int depth) const {
    if (depth >= kMaxAliasDepth) return m_fallback;
    auto it = m_entries.find(role);
    if (it == m_entries.end()) return m_fallback;
    if (it->second.isAlias) return resolveImpl(it->second.aliasOf, depth + 1);
    return it->second.color;
}


/*
    setGradient(const std::string& role, const Gradient& gradient):
    - Params:   const std::string& role, const Gradient& gradient
    - Returns:  void
    - Desc:     Stores a whole gradient under a role so a theme can define it
                once and every element using that gradient role follows a
                palette swap.
*/
void Palette::setGradient(const std::string& role, const Gradient& gradient) {
    m_gradients[role] = gradient;
}


/*
    getGradient(std::string_view role):
    - Params:   std::string_view role
    - Returns:  const Gradient*
    - Desc:     Returns the gradient for a role, or nullptr when the role isn't
                present.
*/
const Gradient* Palette::getGradient(std::string_view role) const {
    auto it = m_gradients.find(role);
    return it == m_gradients.end() ? nullptr : &it->second;
}


/*
    hasGradient(std::string_view role):
    - Params:   std::string_view role
    - Returns:  bool
    - Desc:     True when a gradient is stored under the role.
*/
bool Palette::hasGradient(std::string_view role) const {
    return m_gradients.find(role) != m_gradients.end();
}


/*
    clear():
    - Params:   none
    - Returns:  void
    - Desc:     Removes all color and gradient entries.
*/
void Palette::clear() {
    m_entries.clear();
    m_gradients.clear();
}


/*
    defaultDark():
    - Params:   none
    - Returns:  Palette
    - Desc:     Returns a pre-populated dark palette covering every role UILO's
                built-in widgets read. Intended as a starting point that
                consumers override per role as needed.
*/
Palette Palette::defaultDark() {
    Palette p;
    p.setFallback({255, 0, 255, 255});

    p.set("bg",         {33,  35,  47,  255});
    p.set("panel",      {44,  47,  60,  255});
    p.set("panelAlt",   {40,  44,  56,  255});
    p.set("accent",     {151, 120, 206, 255});
    p.set("accentHover",{171, 140, 226, 255});
    p.set("text",       {235, 238, 245, 255});
    p.set("textDim",    {160, 168, 190, 255});
    p.set("outline",    {80,  84,  100, 255});

    p.setAlias("column.bg", "panel");
    p.setAlias("row.bg",    "panel");

    p.setAlias("text.color",  "text");
    p.setAlias("image.tint",  "text");

    return p;
}


/*
    defaultLight():
    - Params:   none
    - Returns:  Palette
    - Desc:     Returns a pre-populated light palette covering every role
                UILO's built-in widgets read. Intended as a starting point that
                consumers override per role as needed.
*/
Palette Palette::defaultLight() {
    Palette p;
    p.setFallback({255, 0, 255, 255});

    p.set("bg",         {238, 240, 246, 255});
    p.set("panel",      {255, 255, 255, 255});
    p.set("panelAlt",   {245, 247, 252, 255});
    p.set("accent",     {120, 90,  190, 255});
    p.set("accentHover",{140, 110, 210, 255});
    p.set("text",       {30,  32,  44,  255});
    p.set("textDim",    {100, 108, 130, 255});
    p.set("outline",    {200, 205, 220, 255});

    p.setAlias("column.bg",  "panel");
    p.setAlias("row.bg",     "panel");
    p.setAlias("text.color", "text");
    p.setAlias("image.tint", "text");

    return p;
}

}
