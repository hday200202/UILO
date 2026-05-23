#pragma once

#include <chrono>

namespace uilo {

class Timer {
public:
    Timer() = default;

    float restart() {
        auto now = std::chrono::steady_clock::now();
        float secs = std::chrono::duration<float>(now - m_start).count();
        m_start = now;
        return secs;
    }

    float elapsed() const {
        return std::chrono::duration<float>(std::chrono::steady_clock::now() - m_start).count();
    }

private:
    std::chrono::steady_clock::time_point m_start = std::chrono::steady_clock::now();
};

} // namespace uilo