#pragma once

namespace uilo {

/*
    Dimension:
    - Desc:     A size given as absolute pixels, as a percentage of the space the
                parent offers, or as a share of whatever that parent has left
                over. Two flags say which of the three it is, and resolve() turns
                the pair into a concrete number against a parent size. The three
                are kept distinct rather than resolved up front because a share
                cannot be worked out until every sized sibling has taken its own.
    - Write them with the _px, _pct and _flex literals -- 10_px, 50_pct, 1_flex --
      rather than constructing the struct directly.
    - _pct is a percentage of the parent's content area, the same way CSS
      resolves a percentage width, so 50_pct is half the parent whatever its
      siblings do and three of them overflow it.
    - _flex is a share of the space left after the pixel- and percent-sized
      siblings, split between the _flex children in proportion to their values, so
      1_flex on its own takes everything remaining and two of them halve it. This
      is what an unsized Modifier uses, which is why a row of default children
      divides itself evenly. On the cross axis, and inside a scrolling container,
      there is no leftover to divide and a _flex child fills the parent instead.
*/
struct Dimension {
    float value   = 0;
    bool  percent = false;
    bool  flex    = false;

    /* An absolute pixel size: neither derived from the parent nor from what the
       siblings left, so it is the only kind that can be written back to. */
    bool isAbsolute() const { return !percent && !flex; }

    float resolve(float parent) const {
        if (flex)    return parent;
        if (percent) return parent * value / 100.f;
        return value;
    }
};
inline Dimension operator""_px(long double val)          { return {static_cast<float>(val), false, false}; }
inline Dimension operator""_px(unsigned long long val)   { return {static_cast<float>(val), false, false}; }
inline Dimension operator""_pct(long double val)         { return {static_cast<float>(val), true,  false}; }
inline Dimension operator""_pct(unsigned long long val)  { return {static_cast<float>(val), true,  false}; }
inline Dimension operator""_flex(long double val)        { return {static_cast<float>(val), false, true};  }
inline Dimension operator""_flex(unsigned long long val) { return {static_cast<float>(val), false, true};  }

}
