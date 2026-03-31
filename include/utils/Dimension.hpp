#pragma once

namespace uilo {

struct Dimension {
    float value = 0;
    bool percent = false;

    float resolve(float parent) const {
        return percent ? (value * 0.01f * parent) : value;
    }
};

inline Dimension operator""_px(long double v)       { return {static_cast<float>(v), false}; }
inline Dimension operator""_px(unsigned long long v) { return {static_cast<float>(v), false}; }
inline Dimension operator""_pct(long double v)       { return {static_cast<float>(v), true}; }
inline Dimension operator""_pct(unsigned long long v) { return {static_cast<float>(v), true}; }

}