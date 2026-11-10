#pragma once

#include <chrono>
#include <stdexcept>
#include <thread>

using namespace std::chrono_literals;

template <typename TClock = std::chrono::steady_clock>
class TickLoop
{
public:
private:
    typename TClock::time_point start = TClock::now();
    typename TClock::time_point previous = start;

public:
    typename TClock::duration ElapsedTime() const { return TClock::now() - start; }

    typename TClock::duration WaitForTick(typename TClock::duration targetTimeStep = 20ms)
    {
        if (targetTimeStep < TClock::duration::zero())
            throw std::invalid_argument("TickLoop target time step can't be negative.");

        std::this_thread::sleep_until(previous + targetTimeStep);
        const auto now = TClock::now();
        const typename TClock::duration deltaTime = now - previous;
        previous = now;
        return deltaTime;
    }

    void Reset() { start = previous = TClock::now(); }

    template <typename TCallback>
    void Run(TCallback callback, std::chrono::duration<float> targetTimeStep = std::chrono::duration<float>(0.02f))
    {
        Reset();
        const typename TClock::duration targetDuration =
            std::chrono::duration_cast<typename TClock::duration>(targetTimeStep);
        while (callback(std::chrono::duration<float>(WaitForTick(targetDuration)).count(),
                        std::chrono::duration<float>(ElapsedTime()).count())) { }
    }
};