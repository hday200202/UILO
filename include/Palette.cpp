#include "Palette.hpp"

namespace uilo {

namespace {
constexpr int kMaxAliasDepth = 8;
} // namespace

void Palette::set(const std::string& role, Color color) {
    auto& e = m_entries[role];
    e.color   = color;
    e.aliasOf.clear();
    e.isAlias = false;
}

void Palette::setAlias(const std::string& role, const std::string& target) {
    auto& e = m_entries[role];
    e.aliasOf = target;
    e.isAlias = true;
}

bool Palette::has(std::string_view role) const {
    return m_entries.find(role) != m_entries.end();
}

Color Palette::get(std::string_view role) const {
    return resolveImpl(role, 0);
}

Color Palette::resolve(std::string_view role, Color literal) const {
    if (role.empty() || role == "none") return literal;
    auto it = m_entries.find(role);
    if (it == m_entries.end()) return literal;
    if (!it->second.isAlias) return it->second.color;
    return resolveImpl(it->second.aliasOf, 1);
}

Color Palette::resolveImpl(std::string_view role, int depth) const {
    if (depth >= kMaxAliasDepth) return m_fallback;
    auto it = m_entries.find(role);
    if (it == m_entries.end()) return m_fallback;
    if (it->second.isAlias) return resolveImpl(it->second.aliasOf, depth + 1);
    return it->second.color;
}

void Palette::setGradient(const std::string& role, const Gradient& gradient) {
    m_gradients[role] = gradient;
}

const Gradient* Palette::getGradient(std::string_view role) const {
    auto it = m_gradients.find(role);
    return it == m_gradients.end() ? nullptr : &it->second;
}

bool Palette::hasGradient(std::string_view role) const {
    return m_gradients.find(role) != m_gradients.end();
}

void Palette::clear() {
    m_entries.clear();
    m_gradients.clear();
}

// ---------------------------------------------------------------------------
// Defaults
// ---------------------------------------------------------------------------

Palette Palette::defaultDark() {
    Palette p;
    p.setFallback({255, 0, 255, 255});

    // Base
    p.set("bg",         {33,  35,  47,  255});
    p.set("panel",      {44,  47,  60,  255});
    p.set("panelAlt",   {40,  44,  56,  255});
    p.set("accent",     {151, 120, 206, 255});
    p.set("accentHover",{171, 140, 226, 255});
    p.set("text",       {235, 238, 245, 255});
    p.set("textDim",    {160, 168, 190, 255});
    p.set("outline",    {80,  84,  100, 255});

    // Containers
    p.setAlias("column.bg", "panel");
    p.setAlias("row.bg",    "panel");

    // Text / image
    p.setAlias("text.color",  "text");
    p.setAlias("image.tint",  "text");

    return p;
}

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

} // namespace uilo
