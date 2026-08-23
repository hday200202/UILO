#pragma once

#include <string>
#include <utility>

namespace uilo {

/*
    Themed:
    - Desc:     A field that remembers whether anyone set it. An element is
                built before it belongs to a UILO, so the theme cannot be
                consulted at construction -- it is applied later, when the
                element binds. That only works if a field can tell "the caller
                chose this" from "nobody said", which is the whole of what this
                carries: a value, and one bit.

                    Themed<bool> m_bold{false};   // false is the baseline

                    m_bold.set(true);             // explicit; a theme cannot
                                                  // overwrite it
                    m_bold.inherit(proto.m_bold); // fills in only if unset

    - Fields that already have a way to say "unset" do not need this.
      std::optional carries the bit itself, and a colour role uses "" for no
      role, so both are left as they are and merged with the same rule.
    - get() is what the getters return, so nothing outside an Options class ever
      sees this type: converting a field costs one line in its setter and one in
      its getter, and every caller keeps compiling.
*/
template <class T>
class Themed {
public:
    Themed() = default;
    /* The library's baseline, used when neither the caller nor the theme says
       anything. Deliberately implicit, so a field reads as `Themed<bool> x{false}`. */
    Themed(T value) : m_value(std::move(value)) {}

    Themed& set(T value) {
        m_value    = std::move(value);
        m_explicit = true;
        return *this;
    }

    /* Back to following the theme. */
    Themed& clear() {
        m_explicit = false;
        return *this;
    }

    const T& get()        const { return m_value; }
    bool     isExplicit() const { return m_explicit; }

    /*
        inherit(const Themed& prototype):
        - Params:   const Themed& prototype
        - Returns:  void
        - Desc:     Takes the prototype's value if this field was never set.
                    Applied to a whole Options at bind time, this is what makes
                    a theme fill in the gaps without ever overriding a choice
                    the call site made.
        - The explicit bit is not copied: inheriting a value leaves the field
          still following the theme, so a later theme change reaches it too.
    */
    void inherit(const Themed& prototype) {
        if (!m_explicit) m_value = prototype.m_value;
    }

private:
    T    m_value{};
    bool m_explicit = false;
};


/*
    inheritOptional(T& own, const T& prototype):
    - Params:   T& own, const T& prototype
    - Returns:  void
    - Desc:     The same rule for a std::optional field, which already carries
                its own "was it set" bit.
*/
template <class T>
inline void inheritOptional(T& own, const T& prototype) {
    if (!own.has_value()) own = prototype;
}


/*
    inheritRole(std::string& own, const std::string& prototype):
    - Params:   std::string& own, const std::string& prototype
    - Returns:  void
    - Desc:     The same rule for a colour-role field, where "" already means
                "no role was named".
*/
inline void inheritRole(std::string& own, const std::string& prototype) {
    if (own.empty()) own = prototype;
}

} // namespace uilo
