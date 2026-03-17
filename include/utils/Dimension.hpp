#pragma once

namespace uilo {

struct Dimension {
    float value;
    bool percent; // true = percentage (0-100), false = pixels
};

inline Dimension px(float v)  { return {v, false}; }
inline Dimension pct(float v) { return {v, true}; }

inline Dimension operator""_px(long double v)       { return {static_cast<float>(v), false}; }
inline Dimension operator""_px(unsigned long long v) { return {static_cast<float>(v), false}; }
inline Dimension operator""_pct(long double v)       { return {static_cast<float>(v), true}; }
inline Dimension operator""_pct(unsigned long long v){ return {static_cast<float>(v), true}; }

}