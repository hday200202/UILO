#pragma once

namespace uilo {

/*
    Dimension:
    - Desc:     A size given either as absolute pixels or as a percentage of
                whatever
            the parent offers. The `percent` flag says which, and resolve() turns
            the pair into a concrete number against a parent size. Percent
            children share the space left over after their fixed-size siblings
            have taken theirs, which is why the two are kept distinct rather than
            resolved up front.
    - Write them with the _px and _pct literals -- 10_px, 50_pct -- rather than
      constructing the struct directly.
*/
struct Dimension {
    float value = 0;
    bool percent = false;

    float resolve(float parent) const { return percent ? parent * value / 100.f : value; }
};
inline Dimension operator""_px(long double val)         { return {static_cast<float>(val), false}; }
inline Dimension operator""_px(unsigned long long val)  { return {static_cast<float>(val), false}; }
inline Dimension operator""_pct(long double val)        { return {static_cast<float>(val), true};  }
inline Dimension operator""_pct(unsigned long long val) { return {static_cast<float>(val), true};  }

}