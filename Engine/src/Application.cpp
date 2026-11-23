#include "Application.h"

#include "TickLoop.h"

UInt32 Application::Run(const Application::Hooks &hooks)
{
    if (state != State::Stopped)
        return EXIT_FAILURE;

    state = State::Running;

    const TimeDuration targetTimeStep = GetFrequency(60.0);

    UInt32 exitCode = OnStart(hooks);
    if (exitCode == EXIT_SUCCESS && runningLayer != nullptr)
        exitCode = runningLayer->OnStart();

    if (exitCode != EXIT_SUCCESS)
    {
        state = State::Stopped;
        OnStop(exitCode, hooks);
        return exitCode;
    }

    TickLoop tickLoop;
    tickLoop.Run(
        [&](const TimeDuration &delta, const TimeDuration &elapsed)
        {
            time.unscaledDeltaTime = static_cast<float>(delta.count());
            time.deltaTime = time.unscaledDeltaTime * time.timeScale;
            time.unscaledElapsedTime += time.unscaledDeltaTime;
            time.elapsedTime += time.deltaTime;
            time.elapsedFrames += 1;

            if (hooks.preTick) hooks.preTick();

            exitCode = runningLayer != nullptr ?
                runningLayer->OnTick(time) :
                EXIT_SUCCESS;

            if (hooks.postTick) hooks.postTick();

            return state == State::Running && (!hooks.shouldContinue || hooks.shouldContinue());
        },
        targetTimeStep);

    state = State::Stopped;
    OnStop(exitCode, hooks);
    return exitCode;
}

Int32 Application::OnStart(const Application::Hooks &hooks)
{
    if (hooks.onStart)
        hooks.onStart();

    return EXIT_SUCCESS;
}

bool Application::RequestStop()
{
    if (state != State::Running)
        return false;

    state = State::Stopping;
    return true;
}

void Application::OnStop(const Int32 exitCode, const Application::Hooks &hooks)
{
    if (runningLayer != nullptr)
        runningLayer->OnTransitionFrom();

    if (hooks.onStop)
        hooks.onStop();

    runningLayer = nullptr;
}
