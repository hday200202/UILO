#include "Palette.hpp"

namespace uilo {

namespace {
constexpr int kMaxAliasDepth = 8;
}


/*
    set(const std::string& role, Color color):
    - Params:   const std::string& role, Color color
    - Returns:  void
    - Desc:     Assigns a direct color to a role, overwriting any existing entry
                including an alias.
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
                reaches a direct color. Returns the fallback color when the role
                is missing or the alias depth cap is reached, which breaks
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

}
