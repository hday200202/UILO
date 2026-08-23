#pragma once

#include <any>
#include <string>
#include <string_view>
#include <typeindex>
#include <unordered_map>
#include <utility>

#include "../Palette.hpp"
#include "Themed.hpp"

namespace uilo {

class Theme;

/*
    installDefaultTheme(Theme& theme):
    - Params:   Theme& theme
    - Returns:  void
    - Desc:     Fills a theme with everything Defaults.hpp declares. Declared
                here and defined in Defaults.cpp, so a UILO can build its theme
                without this header -- which every themed type includes -- having
                to pull in every element type in turn.
*/
void installDefaultTheme(Theme& theme);


/*
    Theme:
    - Desc:     What everything looks like before anyone says otherwise: the
                palette, and a prototype for every Modifier and *Options type in
                the library. A UILO owns one, so two of them can look different
                in the same process -- which is what a Wt session needs.

                    ui.getTheme().palette().set("accent", {90, 140, 255, 255});
                    ui.setTheme(myTheme);

    - The prototypes live in Defaults.hpp. That file is the whole of the
      library's look, and editing it is the intended way to restyle an
      application wholesale.
    - A theme is applied when an element binds to its UILO, not when the element
      is constructed -- an element is usually built before the UILO exists. Only
      fields the call site never set are filled in, so a theme fills gaps and
      never overrides a choice. Changing a UILO's theme re-applies it to
      everything already built, so restyling needs no rebuild.
    - Named roles work the same way the palette's do, for shape rather than
      colour. A theme can register any number of variants per type under a name,
      and constructing a type with that name records it so the right prototype
      is found at bind time:

            TextOptions h1 = theme.edit<TextOptions>("h1");
            h1.setCharSize(42);

            text(Modifier("h1"), TextOptions("h1"));

      Modifier and *Options roles are looked up separately, so the same name can
      mean a size on one and a colour on the other. An unknown role is not an
      error: it falls back to the type's default, the way an unknown CSS class
      leaves an element unstyled. Ask hasRole() when a typo would matter.
*/
class Theme {
public:
    Theme() = default;

    Theme&         setPalette(Palette palette) { m_palette = std::move(palette); return *this; }
    Palette&       palette()                   { return m_palette; }
    const Palette& palette() const             { return m_palette; }

    /* The stored prototype, to change in place:

           theme.edit<TextboxOptions>().setRounding(8.f);

       Creates it from the type's default the first time, so a theme can be
       adjusted a line at a time without rebuilding a prototype by hand. */
    template <class T> T& edit(std::string_view role = "");

    /* Null when this theme says nothing about that type and role. */
    template <class T> const T* lookup(std::string_view role) const;

    /* What an element with this role inherits: the named prototype, with the
       type's default filling in whatever that role does not mention. */
    template <class T> T cascade(std::string_view role) const;

    template <class T> bool hasRole(std::string_view role) const;

private:
    /* One map per type, type-erased so this header needs to know none of them.
       std::any copies what it holds, which is what keeps a Theme a value: two
       themes never share a prototype. */
    std::unordered_map<std::type_index, std::any> m_roles;
    Palette                                       m_palette;
};


/*
    edit(std::string_view role):
    - Params:   std::string_view role
    - Returns:  T& -- the stored prototype, to change in place
    - Desc:     The prototype for a type and role, created empty on first use.
    - A role holds only what it changes, never a copy of the default. Seeding it
      from the default would look convenient and then go stale: the copy would
      keep whatever the default held at that moment, so a later change to the
      default would reach every plain element and silently skip the ones that
      named a role. cascade() composes the two at read time instead, which is
      what keeps the two in step.
    - The reference is invalidated by a later edit() of the same type, so change
      one thing and let go of it.
*/
template <class T>
T& Theme::edit(std::string_view role) {
    using Roles = std::unordered_map<std::string, T>;

    std::any& slot = m_roles[std::type_index(typeid(T))];
    if (!slot.has_value()) slot.emplace<Roles>();

    Roles& roles = std::any_cast<Roles&>(slot);
    return roles.emplace(std::string(role), T{}).first->second;
}


/*
    lookup(std::string_view role):
    - Params:   std::string_view role
    - Returns:  const T* -- null when unset
    - Desc:     The prototype this theme holds for a type and role. The pointer
                stays valid until that role is redefined or the theme is
                replaced, which is long enough to inherit from.
*/
template <class T>
const T* Theme::lookup(std::string_view role) const {
    using Roles = std::unordered_map<std::string, T>;

    const auto slot = m_roles.find(std::type_index(typeid(T)));
    if (slot == m_roles.end()) return nullptr;

    const Roles* roles = std::any_cast<Roles>(&slot->second);
    if (!roles) return nullptr;

    const auto found = roles->find(std::string(role));
    return found == roles->end() ? nullptr : &found->second;
}


/*
    cascade(std::string_view role):
    - Params:   std::string_view role
    - Returns:  T -- the role over the type's default
    - Desc:     What an element with this role inherits at bind time. A role says
                only what it changes, so the type's default has to fill in the
                rest: "primary" names a fill and nothing else, and still has to
                arrive carrying the radius every button shares.
    - Composed on every read rather than when the role was defined. edit() seeds
      a new role from the default as a convenience, but that is a snapshot --
      editing the default afterwards has to reach the roles built on it, or
      changing one shared value would quietly skip every element that named a
      role. That is exactly what a stylesheet does: the class refines the
      element's own style rather than replacing it.
    - An unknown role is not an error; it comes back as the plain default.
*/
template <class T>
T Theme::cascade(std::string_view role) const {
    const T* base = lookup<T>("");

    const T* named = role.empty() ? nullptr : lookup<T>(role);
    if (!named) return base ? *base : T{};

    /* The role's own settings are explicit, so filling from the default cannot
       overwrite them -- it only reaches what the role stayed quiet about. */
    T result = *named;
    if (base) result.inheritFrom(*base);
    return result;
}


/*
    hasRole(std::string_view role):
    - Params:   std::string_view role
    - Returns:  bool
    - Desc:     Whether this theme defines that role for T. Worth asking before
                relying on one, since binding is silent about a name it does not
                know.
*/
template <class T>
bool Theme::hasRole(std::string_view role) const {
    return lookup<T>(role) != nullptr;
}

} // namespace uilo


/*
    UILO_THEMED:
    - Desc:     The two constructors and the role accessor every themed type
                shares, spelled once. Takes the type it is expanded in.

                    T()          the type's baseline; the theme fills it in
                                 when the element binds
                    T("h1")      the same, but recording that it wants the
                                 theme's "h1" variant

                A type opts in by writing UILO_THEMED(TextOptions) where its
                default constructor used to be. Its member initialisers stay
                exactly as they were and become the baseline that shows through
                wherever neither the caller nor the theme has an opinion.
*/
#define UILO_THEMED(T)                                                    \
    T() = default;                                                        \
    explicit T(::std::string_view role) : m_themeRole(role) {}            \
    const ::std::string& getThemeRole() const { return m_themeRole; }     \
    T& setThemeRole(::std::string_view role) {                            \
        m_themeRole = role;                                               \
        return *this;                                                     \
    }
