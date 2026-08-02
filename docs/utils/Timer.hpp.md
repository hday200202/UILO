# Timer.hpp

`include/utils/Timer.hpp`

[← index](../README.md)

## Types

- [Timer](#timer)

---

### Timer

A monotonic stopwatch over steady_clock, used for frame timing and the caret blink. steady_clock rather than system_clock so a change to the machine's wall clock cannot make an interval come out negative.
