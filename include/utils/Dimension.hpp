#pragma once

namespace uilo {

/*
    Dimension:
    - value: the actual value of the dimension, 
      either in pixels or percentage
    - percent: whether the value is a percentage 
      of the parent dimension or an absolute pixel 
      value
    - resolve(parent): a method to calculate the 
      actual pixel value based on the parent 
      dimension if it's a percentage, or return 
      the value directly if it's in pixels
*/
struct Dimension {
    float value = 0;
    bool percent = false;

    float resolve(float parent) const { return percent ? parent * value / 100.f : value; }
};

// Literals to make it easier to specify dimensions in code, e.g. 10_px or 50_pct
inline Dimension operator""_px(long double val)         { return {static_cast<float>(val), false}; }
inline Dimension operator""_px(unsigned long long val)  { return {static_cast<float>(val), false}; }
inline Dimension operator""_pct(long double val)        { return {static_cast<float>(val), true};  }
inline Dimension operator""_pct(unsigned long long val) { return {static_cast<float>(val), true};  }

}