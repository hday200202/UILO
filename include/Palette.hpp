#pragma once

#include "utils/Color.hpp"
#include "utils/Gradient.hpp"

#include <string>
#include <string_view>
#include <unordered_map>

namespace uilo {

/*
    Palette:
    - Desc: A flat name->Color map with alias and named-gradient support,
            owned by UILO. Elements resolve role strings to colors at draw
            time via UILO::getPalette(). An empty role or "none" means "no
            role" and falls back to the element's literal color; an
            unresolved non-empty role falls back to the literal, then to
            the palette's fallback color. Aliases let one role point at
            another so a small base of colors drives many widget-specific
            roles, with cycles broken after a depth cap.
*/
class Palette {
public:
    Palette() = default;

    void set(const std::string& role, Color color);
    void setAlias(const std::string& role, const std::string& target);
    Color get(std::string_view role) const;
    bool has(std::string_view role) const;
    Color resolve(std::string_view role, Color literal) const;

    void  setFallback(Color c) { m_fallback = c; }
    Color getFallback() const  { return m_fallback; }

    void setGradient(const std::string& role, const Gradient& gradient);
    const Gradient* getGradient(std::string_view role) const;
    bool hasGradient(std::string_view role) const;

    void clear();

    static Palette defaultDark();
    static Palette defaultLight();

private:
    struct Entry {
        Color       color { 0, 0, 0, 0 };
        std::string aliasOf;
        bool        isAlias = false;
    };

    Color resolveImpl(std::string_view role, int depth) const;

    struct StringHash {
        using is_transparent = void;
        size_t operator()(std::string_view s) const noexcept {
            return std::hash<std::string_view>{}(s);
        }
        size_t operator()(const std::string& s) const noexcept {
            return std::hash<std::string_view>{}(s);
        }
        size_t operator()(const char* s) const noexcept {
            return std::hash<std::string_view>{}(s);
        }
    };
    struct StringEq {
        using is_transparent = void;
        bool operator()(std::string_view a, std::string_view b) const noexcept { return a == b; }
        bool operator()(std::string_view a, const std::string& b) const noexcept { return a == b; }
        bool operator()(const std::string& a, std::string_view b) const noexcept { return a == b; }
        bool operator()(const std::string& a, const std::string& b) const noexcept { return a == b; }
    };

    std::unordered_map<std::string, Entry, StringHash, StringEq> m_entries;
    std::unordered_map<std::string, Gradient, StringHash, StringEq> m_gradients;
    Color m_fallback { 255, 0, 255, 255 };
};

}
