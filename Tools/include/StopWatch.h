#pragma once

#include <chrono>

using namespace std::chrono_literals;

template<typename TClock = std::chrono::steady_clock>
class Stopwatch
{
    using TimePoint = typename TClock::time_point;
    using Duration = typename TClock::duration;

    TimePoint start = TClock::now();

public:
    [[nodiscard]] Duration CurrentDuration() { return TClock::now() - start; }

    Stopwatch() { Reset(); }

    Duration Reset()
    {
        Duration duration = CurrentDuration();
        start = TClock::now();
        return duration;
    }
};