#pragma once

#include <functional>
#include <type_traits>
#include <utility>
#include "../../include/utils/Math.hpp"
#include "../utils/Utils.hpp"
#include "../utils/Material.hpp"

namespace uilo {

class Element; // fwd: callbacks receive a self-pointer

// Canonical storage signatures: every handler receives a pointer to the
// element that fired it, so lambdas can mutate the element they're
// attached to without capturing it from the outside. The public setters
// below accept many friendlier shapes (no-arg, generic `Element*`,
// element-typed `Button*` etc.) and wrap them down to these.
using FuncPtr       = std::function<void(Element*)>;
using ScrollFuncPtr = std::function<void(Element*, float)>;

namespace detail {

// --- Lambda / functor first-argument introspection ------------------------
// Used to auto-wrap user-supplied callables so they can be written as any
// of `[](){}`, `[](Element*){}`, or `[](Button*){}` without ceremony.
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

// Plain function pointers
template <class R>             struct cb_traits<R(*)()>           { static constexpr int arity = 0; using arg0 = void; };
template <class R, class A>    struct cb_traits<R(*)(A)>          { static constexpr int arity = 1; using arg0 = A; };
template <class R, class A, class B>
struct cb_traits<R(*)(A, B)>                                       { static constexpr int arity = 2; using arg0 = A; using arg1 = B; };

template <class F> using cb_arg0 = typename cb_traits<std::decay_t<F>>::arg0;
template <class F> inline constexpr int cb_arity = cb_traits<std::decay_t<F>>::arity;

// Wrap a click-style callable into FuncPtr.
template <class F>
inline FuncPtr makeClickCb(F&& f) {
    if constexpr (cb_arity<F> == 0) {
        return [fn = std::forward<F>(f)](Element*) mutable { fn(); };
    } else {
        using Arg = cb_arg0<F>;
        static_assert(std::is_pointer_v<Arg>,
            "Click/hover callbacks must take no args, or a single pointer "
            "to the element type (e.g. `[](Element*){}` or `[](Button*){}`).");
        using ArgPtr = Arg; // already a pointer type
        return [fn = std::forward<F>(f)](Element* e) mutable {
            fn(static_cast<ArgPtr>(e));
        };
    }
}

// Wrap a scroll-style callable into ScrollFuncPtr.
template <class F>
inline ScrollFuncPtr makeScrollCb(F&& f) {
    if constexpr (cb_arity<F> == 1) {
        // legacy `[](float delta){}` — no self pointer
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

class Modifier {
public:
    Modifier() = default;

    Modifier& setWidth(Dimension dim);
    Modifier& setHeight(Dimension dim);
    Modifier& setAlign(Align alignment);

    // Click / hover / scroll setters auto-wrap a wide range of lambdas:
    //   .setOnLeftClick([](){ ... })                 // legacy no-arg
    //   .setOnLeftClick([](Element* self){ ... })    // generic self-ptr
    //   .setOnLeftClick([](Button*  self){ ... })    // typed self-ptr
    template <class F> Modifier& setOnLeftClick(F&& f)  { m_onLeftClick  = detail::makeClickCb(std::forward<F>(f));  return *this; }
    template <class F> Modifier& setOnRightClick(F&& f) { m_onRightClick = detail::makeClickCb(std::forward<F>(f));  return *this; }

    // Edge-triggered hover callbacks. `onHoverEnter` fires once when the
    // cursor first enters the element bounds; `onHoverExit` fires once
    // when it leaves. There is intentionally no continuous "is-hovered"
    // callback — handlers should react to the transitions and toggle
    // state on the element itself if a persistent visual change is
    // wanted.
    template <class F> Modifier& setOnHoverEnter(F&& f) { m_onHoverEnter = detail::makeClickCb(std::forward<F>(f)); return *this; }
    template <class F> Modifier& setOnHoverExit(F&& f)  { m_onHoverExit  = detail::makeClickCb(std::forward<F>(f)); return *this; }
    // Back-compat alias: legacy `setOnHover` callers get enter semantics
    // (which is how it had always behaved on Element — see Element.cpp).
    template <class F> Modifier& setOnHover(F&& f)      { return setOnHoverEnter(std::forward<F>(f)); }

    template <class F> Modifier& setOnScroll(F&& f)     { m_onScroll = detail::makeScrollCb(std::forward<F>(f));     return *this; }

    // Per-frame lifecycle hooks. `onUpdateStart` fires at the top of every
    // update tick (before layout/state is recomputed); `onUpdateEnd` fires
    // after. Both receive the element self-pointer so handlers can read
    // current bounds or mutate options based on per-frame state (e.g.
    // `if (r->isDragging()) r->getOptions().setColor(...)`).
    template <class F> Modifier& setOnUpdateStart(F&& f) { m_onUpdateStart = detail::makeClickCb(std::forward<F>(f)); return *this; }
    template <class F> Modifier& setOnUpdateEnd(F&& f)   { m_onUpdateEnd   = detail::makeClickCb(std::forward<F>(f)); return *this; }

    Modifier& setOuterPadding(float padding);
    Modifier& setVisible(bool visible);
    Modifier& setFreePosition(const Vec2f& freePos);
    Modifier& setMaterial(const Material& m);

    Dimension getWidth()                const;
    Dimension getHeight()               const;
    Align getAlign()                    const;
    const FuncPtr& getOnLeftClick()     const;
    const FuncPtr& getOnRightClick()    const;
    const FuncPtr& getOnHoverEnter()    const;
    const FuncPtr& getOnHoverExit()     const;
    const FuncPtr& getOnUpdateStart()   const;
    const FuncPtr& getOnUpdateEnd()     const;
    const ScrollFuncPtr& getOnScroll()  const;
    float getOuterPadding()             const;
    bool getVisible()                   const;
    Vec2f getFreePosition() const;
    const Material& getMaterial()       const;

private:
    Dimension m_width                   = 100_pct;
    Dimension m_height                  = 100_pct;
    Align m_align                       = Align::Left | Align::Top;
    FuncPtr m_onLeftClick;
    FuncPtr m_onRightClick;
    FuncPtr m_onHoverEnter;
    FuncPtr m_onHoverExit;
    FuncPtr m_onUpdateStart;
    FuncPtr m_onUpdateEnd;
    ScrollFuncPtr m_onScroll;
    float m_outerPadding                = 0.f;
    bool m_visible                      = true;
    Vec2f m_freePosition = {0.f, 0.f};
    Material m_material;
};

}