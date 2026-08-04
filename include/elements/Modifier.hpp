#pragma once

#include <functional>
#include <type_traits>
#include <utility>
#include "../../include/utils/Math.hpp"
#include "../utils/Utils.hpp"
#include "../utils/Material.hpp"
#include "../utils/Theme.hpp"
#include "../utils/Cursor.hpp"

namespace uilo {

class Element;

/*
    FuncPtr, ScrollFuncPtr:
    - Desc: Canonical storage signatures for element callbacks. Every handler
            receives a pointer to the element that fired it, so a lambda can
            mutate the element it is attached to without having captured it from
            outside. The public setters accept friendlier shapes -- no argument,
            a generic Element*, or a typed Button* -- and wrap them down to these.
*/
using FuncPtr       = std::function<void(Element*)>;
using ScrollFuncPtr = std::function<void(Element*, float)>;

namespace detail {

/*
    cb_traits:
    - Desc:     First-argument introspection for a callable, so a user-supplied
                handler can be written as `[](){}`, `[](Element*){}` or
                `[](Button*){}` without ceremony. Reports the arity and the
                first parameter type; specialised for member operator() in both
                const and non-const forms, and for plain function pointers.
*/
template <class T> struct cb_traits : cb_traits<decltype(&T::operator())> {};

template <class R, class C>
struct cb_traits<R(C::*)() const>            { static constexpr int arity = 0; using arg0 = void; };
template <class R, class C>
struct cb_traits<R(C::*)()>                  { static constexpr int arity = 0; using arg0 = void; };
template <class R, class C, class A>
struct cb_traits<R(C::*)(A) const>           { static constexpr int arity = 1; using arg0 = A; };
template <class R, class C, class A>
struct cb_traits<R(C::*)(A)>                 { static constexpr int arity = 1; using arg0 = A; };
template <class R, class C, class A, class B>
struct cb_traits<R(C::*)(A, B) const>        { static constexpr int arity = 2; using arg0 = A; using arg1 = B; };
template <class R, class C, class A, class B>
struct cb_traits<R(C::*)(A, B)>              { static constexpr int arity = 2; using arg0 = A; using arg1 = B; };

template <class R>             struct cb_traits<R(*)()>           { static constexpr int arity = 0; using arg0 = void; };
template <class R, class A>    struct cb_traits<R(*)(A)>          { static constexpr int arity = 1; using arg0 = A; };
template <class R, class A, class B>
struct cb_traits<R(*)(A, B)>                                       { static constexpr int arity = 2; using arg0 = A; using arg1 = B; };

template <class F> using cb_arg0 = typename cb_traits<std::decay_t<F>>::arg0;
template <class F> inline constexpr int cb_arity = cb_traits<std::decay_t<F>>::arity;


/*
    makeClickCb(F&& f):
    - Params:   F&& f
    - Returns:  FuncPtr
    - Desc:     Wraps a click- or hover-style callable into the canonical
                signature. A no-argument callable is called with the self-
                pointer discarded; a one-argument callable has the pointer cast
                to the type it asked for, which is what lets a handler take
                Button* directly. Anything else fails a static_assert with the
                accepted shapes spelled out.
*/
template <class F>
inline FuncPtr makeClickCb(F&& f) {
    if constexpr (cb_arity<F> == 0) {
        return [fn = std::forward<F>(f)](Element*) mutable { fn(); };
    } else {
        using Arg = cb_arg0<F>;
        static_assert(std::is_pointer_v<Arg>,
            "Click/hover callbacks must take no args, or a single pointer "
            "to the element type (e.g. `[](Element*){}` or `[](Button*){}`).");
        using ArgPtr = Arg;
        return [fn = std::forward<F>(f)](Element* e) mutable {
            fn(static_cast<ArgPtr>(e));
        };
    }
}


/*
    makeScrollCb(F&& f):
    - Params:   F&& f
    - Returns:  ScrollFuncPtr
    - Desc:     Wraps a scroll-style callable into the canonical signature. A
                one-argument callable is taken as `(float delta)` and called
                with the self-pointer discarded; a two-argument one gets the
                pointer cast to the type it asked for.
*/
template <class F>
inline ScrollFuncPtr makeScrollCb(F&& f) {
    if constexpr (cb_arity<F> == 1) {
        return [fn = std::forward<F>(f)](Element*, float d) mutable { fn(d); };
    } else {
        using Arg = cb_arg0<F>;
        static_assert(std::is_pointer_v<Arg>,
            "Scroll callbacks must take `(float)` or `(ElementPtr, float)`.");
        return [fn = std::forward<F>(f)](Element* e, float d) mutable {
            fn(static_cast<Arg>(e), d);
        };
    }
}

} // namespace detail


/*
    Modifier:
    - Desc:     The layout and event half of an element, kept separate from the
                per-widget Options that describe how it is drawn. Carries size,
                alignment, outer padding, visibility, free position, material,
                and the callback set: clicks, edge-triggered hover, scroll, and
                the two per-frame lifecycle hooks. Every setter returns *this so
                a modifier reads as one declaration at the call site.
    - There is deliberately no continuous "is hovered" callback: handlers react
      to the enter and exit transitions and keep any persistent visual change on
      the element themselves.
*/
class Modifier {
public:
    Modifier() = default;

    Modifier& setWidth(Dimension dim);
    Modifier& setHeight(Dimension dim);
    Modifier& setAlign(Align alignment);

    template <class F> Modifier& setOnLeftClick(F&& f);
    template <class F> Modifier& setOnRightClick(F&& f);
    template <class F> Modifier& setOnHoverEnter(F&& f);
    template <class F> Modifier& setOnHoverExit(F&& f);
    template <class F> Modifier& setOnHover(F&& f);
    template <class F> Modifier& setOnScroll(F&& f);
    template <class F> Modifier& setOnUpdateStart(F&& f);
    template <class F> Modifier& setOnUpdateEnd(F&& f);

    Modifier& setVisible(bool visible);
    Modifier& ignoreScroll(bool ignore);
    Modifier& setFreePosition(const Vec2f& freePos);
    Modifier& setMaterial(const Material& m);
    Modifier& setCursor(CursorType cursor);
    Modifier& clearCursor();

    Dimension            getWidth()          const;
    Dimension            getHeight()         const;
    Align                getAlign()          const;
    const FuncPtr&       getOnLeftClick()    const;
    const FuncPtr&       getOnRightClick()   const;
    const FuncPtr&       getOnHoverEnter()   const;
    const FuncPtr&       getOnHoverExit()    const;
    const FuncPtr&       getOnUpdateStart()  const;
    const FuncPtr&       getOnUpdateEnd()    const;
    const ScrollFuncPtr& getOnScroll()       const;
    bool                 getVisible()        const;
    bool                 getIgnoreScroll()   const;
    Vec2f                getFreePosition()   const;
    const Material&      getMaterial()       const;
    CursorType           getCursor()         const;
    bool                 hasCursor()         const;

private:
    Dimension m_width  = 100_pct;
    Dimension m_height = 100_pct;
    Align     m_align  = Align::Left | Align::Top;

    FuncPtr       m_onLeftClick;
    FuncPtr       m_onRightClick;
    FuncPtr       m_onHoverEnter;
    FuncPtr       m_onHoverExit;
    FuncPtr       m_onUpdateStart;
    FuncPtr       m_onUpdateEnd;
    ScrollFuncPtr m_onScroll;

    bool                 m_visible      = true;
    bool                 m_ignoreScroll = false;
    Vec2f                m_freePosition = {0.f, 0.f};
    Material             m_material;
    std::optional<CursorType> m_cursor;
};


/*
    setOnLeftClick(F&& f):
    - Params:   F&& f
    - Returns:  Modifier&
    - Desc:     Left-click handler. Accepts a no-argument lambda, one taking a
                generic Element*, or one taking the concrete element type, and
                wraps it down to the canonical signature.
*/
template <class F>
Modifier& Modifier::setOnLeftClick(F&& f) {
    m_onLeftClick = detail::makeClickCb(std::forward<F>(f));
    return *this;
}


/*
    setOnRightClick(F&& f):
    - Params:   F&& f
    - Returns:  Modifier&
    - Desc:     Right-click handler, accepting the same callable shapes as
                setOnLeftClick.
*/
template <class F>
Modifier& Modifier::setOnRightClick(F&& f) {
    m_onRightClick = detail::makeClickCb(std::forward<F>(f));
    return *this;
}


/*
    setOnHoverEnter(F&& f):
    - Params:   F&& f
    - Returns:  Modifier&
    - Desc:     Fires once when the cursor first enters the element's bounds.
*/
template <class F>
Modifier& Modifier::setOnHoverEnter(F&& f) {
    m_onHoverEnter = detail::makeClickCb(std::forward<F>(f));
    return *this;
}


/*
    setOnHoverExit(F&& f):
    - Params:   F&& f
    - Returns:  Modifier&
    - Desc:     Fires once when the cursor leaves the element's bounds.
*/
template <class F>
Modifier& Modifier::setOnHoverExit(F&& f) {
    m_onHoverExit = detail::makeClickCb(std::forward<F>(f));
    return *this;
}


/*
    setOnHover(F&& f):
    - Params:   F&& f
    - Returns:  Modifier&
    - Desc:     Back-compatible alias for setOnHoverEnter, which is how the
                older single hover callback always behaved.
*/
template <class F>
Modifier& Modifier::setOnHover(F&& f) {
    return setOnHoverEnter(std::forward<F>(f));
}


/*
    setOnScroll(F&& f):
    - Params:   F&& f
    - Returns:  Modifier&
    - Desc:     Scroll handler. Accepts either `(float delta)` or `(ElementPtr,
                float delta)`.
*/
template <class F>
Modifier& Modifier::setOnScroll(F&& f) {
    m_onScroll = detail::makeScrollCb(std::forward<F>(f));
    return *this;
}


/*
    setOnUpdateStart(F&& f):
    - Params:   F&& f
    - Returns:  Modifier&
    - Desc:     Fires at the top of every update tick, before layout and state
                are recomputed. Receives the element, so a handler can read the
                current bounds or drive options from per-frame state.
*/
template <class F>
Modifier& Modifier::setOnUpdateStart(F&& f) {
    m_onUpdateStart = detail::makeClickCb(std::forward<F>(f));
    return *this;
}


/*
    setOnUpdateEnd(F&& f):
    - Params:   F&& f
    - Returns:  Modifier&
    - Desc:     Fires at the end of every update tick, once layout has run.
*/
template <class F>
Modifier& Modifier::setOnUpdateEnd(F&& f) {
    m_onUpdateEnd = detail::makeClickCb(std::forward<F>(f));
    return *this;
}

}
