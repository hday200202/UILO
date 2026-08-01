#pragma once

#include <chrono>

namespace uilo {

/*
    Timer:
    - Desc: A monotonic stopwatch over steady_clock, used for frame timing and
            the caret blink. steady_clock rather than system_clock so a change to
            the machine's wall clock cannot make an interval come out negative.
*/
class Timer {
public:
    Timer() = default;

    /*
        restart():
        - Params:   none
        - Returns:  float -- seconds since the last restart
        - Desc:     Reports the elapsed time and begins a new interval, which is
                    how a frame loop gets its delta.
    */
    float restart() {
        auto now = std::chrono::steady_clock::now();
        float secs = std::chrono::duration<float>(now - m_start).count();
        m_start = now;
        return secs;
    }

    /*
        elapsed():
        - Params:   none
        - Returns:  float -- seconds since the last restart
        - Desc:     Reports the elapsed time without disturbing the interval.
    */
    float elapsed() const {
        return std::chrono::duration<float>(std::chrono::steady_clock::now() - m_start).count();
    }

private:
    std::chrono::steady_clock::time_point m_start = std::chrono::steady_clock::now();
};

} // namespace uilo