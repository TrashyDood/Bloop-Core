#include "Application.h"

#include "TickLoop.h"

UInt32 Application::SetActiveLayer(Application::Layer *newLayer, Application::Layer *&outPreviousLayer)
{
    if (newLayer == runningLayer)
        return EXIT_FAILURE;

    Layer *previousLayer = runningLayer;

    if (runningLayer != nullptr)
        runningLayer->OnTransitionFrom();

    runningLayer = newLayer;

    if (runningLayer != nullptr)
        runningLayer->OnTransitionTo();

    outPreviousLayer = previousLayer;
    return EXIT_SUCCESS;
}

UInt32 Application::Run(const Application::Hooks &hooks)
{
    if (state != State::Stopped)
        return EXIT_FAILURE;

    state = State::Running;

    const TimeDuration targetTimeStep = GetFrequency(60.0);

    UInt32 exitCode = runningLayer != nullptr ?
        runningLayer->OnStart() :
        EXIT_SUCCESS;

    if (exitCode != EXIT_SUCCESS)
    {
        state = State::Stopped;
        OnStop();
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

            if (hooks.preTick) hooks.preTick();

            exitCode = runningLayer != nullptr ?
                runningLayer->OnTick(time) :
                EXIT_SUCCESS;

            if (hooks.postTick) hooks.postTick();

            return state == State::Running && (!hooks.shouldContinue || hooks.shouldContinue());
        },
        targetTimeStep);

    state = State::Stopped;
    OnStop();
    return exitCode;
}

void Application::RequestStop()
{
    if (state != State::Running)
        return;

    state = State::Stopping;
}

void Application::OnStop()
{
    Application::Layer *previousLayer = nullptr;
    SetActiveLayer(nullptr, previousLayer);
}
