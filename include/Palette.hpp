#pragma once

#include "utils/Color.hpp"
#include "utils/Gradient.hpp"

#include <string>
#include <string_view>
#include <unordered_map>

namespace uilo {

// ---------------------------------------------------------------------------
// Palette
//
// A flat name->Color map with simple alias support. Owned by UILO; elements
// resolve role strings to colors at draw time via UILO::getPalette().
//
// Empty role or the literal "none" means "no role" — the element falls back
// to its literal color (whatever it was given via setColor). A non-empty
// role that doesn't resolve falls back to the literal color too, then to
// the palette's fallback color if the literal is fully transparent.
//
// Aliases let one role point at another so a small base of colors can drive
// many widget-specific roles. Cycles are detected and broken.
// ---------------------------------------------------------------------------
class Palette {
public:
    Palette() = default;

    // Direct color assignment. Overwrites any existing entry (including alias).
    void set(const std::string& role, Color color);

    // Make `role` resolve to whatever `target` resolves to. Overwrites any
    // existing entry. Cycles are tolerated at resolution time (returns the
    // fallback after a depth cap).
    void setAlias(const std::string& role, const std::string& target);

    // Returns the color for `role`, walking alias chains. If unresolved,
    // returns the fallback color.
    Color get(std::string_view role) const;

    // True iff role exists in the map (as a color or alias). Does not follow
    // aliases to verify they ultimately resolve.
    bool has(std::string_view role) const;

    // Convenience used by render paths. If role is empty or "none", returns
    // `literal`. Otherwise returns the palette resolution (which itself
    // falls back to the palette's fallback color if the role chain dies).
    Color resolve(std::string_view role, Color literal) const;

    void  setFallback(Color c) { m_fallback = c; }
    Color getFallback() const  { return m_fallback; }

    // ---- Named gradients ---------------------------------------------------
    // Whole gradients can be palette entries too, so a theme can define e.g.
    // "hero" once and every element using setGradientRole("hero") follows a
    // palette swap. Individual GradientStop roles inside the gradient are
    // still resolved against the color entries above at draw time.
    void setGradient(const std::string& role, const Gradient& gradient);
    // Returns nullptr when the role isn't present.
    const Gradient* getGradient(std::string_view role) const;
    bool hasGradient(std::string_view role) const;

    void clear();

    // Pre-populated palettes that cover every role UILO's built-in widgets
    // read. Consumers can use these as a starting point and override
    // specific roles as needed.
    static Palette defaultDark();
    static Palette defaultLight();

private:
    struct Entry {
        Color       color { 0, 0, 0, 0 };
        std::string aliasOf;          // empty = direct color
        bool        isAlias = false;
    };

    Color resolveImpl(std::string_view role, int depth) const;

    // Transparent hash/eq so lookups by string_view don't have to
    // construct a std::string (and heap-allocate) on every call. The
    // palette is on the per-frame hot path — one allocation per element
    // per frame adds up fast.
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
    Color m_fallback { 255, 0, 255, 255 }; // unmistakable magenta
};

} // namespace uilo
