#pragma once

#include <chrono>
#include <stdexcept>
#include <thread>
#include "Shared.h"

using namespace std::chrono_literals;

template <typename TClock = std::chrono::steady_clock>
class TickLoop
{
    using TimePoint = typename TClock::time_point;
    using Duration = typename TClock::duration;

    TimePoint start = TClock::now(),
        previous = start;

public:
    Duration GetElapsedTime() const { return TClock::now() - start; }
    Duration GetDelta() const { return TClock::now() - previous; }

    void Reset() { start = previous = TClock::now(); }

    const Duration WaitForTick(const Duration &targetTimeStep = 20ms)
    {
        if (targetTimeStep < TClock::duration::zero())
            throw std::invalid_argument("TickLoop target time step can't be negative.");

        const auto deadline = previous + targetTimeStep;
        const auto remaining = deadline - TClock::now();

        if (remaining > std::chrono::milliseconds(1))
            std::this_thread::sleep_until(deadline - std::chrono::milliseconds(1));
        while (TClock::now() < deadline) { }

        const auto tick = TClock::now();
        const auto delta = tick - previous;
        previous = tick;

        return delta;
    }

    template <typename TCallback, typename TRep, typename TPeriod>
    void Run(const TCallback callback, std::chrono::duration<TRep, TPeriod> &targetTimeStep)
    {
        #ifdef _WIN32
        TimerResolution timerResolution(targetTimeStep > std::chrono::duration<TRep, TPeriod>::zero());
        #endif
        
        Reset();

        using CallbackDuration = std::chrono::duration<TRep, TPeriod>;
        CallbackDuration delta;
        CallbackDuration elapsed;
        
        do
        {
            elapsed = std::chrono::duration_cast<CallbackDuration>(GetElapsedTime());
            delta = std::chrono::duration_cast<CallbackDuration>(
                WaitForTick(std::chrono::duration_cast<Duration>(targetTimeStep)));
        } while (callback(delta, elapsed));
    }
};