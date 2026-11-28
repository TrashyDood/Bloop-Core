#pragma once

#include "Shared.h"
#include "IOUtils.h"

#include <functional>
#include <utility>

class Application
{
public:
    struct Time
    {
        float unscaledDeltaTime = 0.f,
            deltaTime = 0.f,
            unscaledElapsedTime = 0.f,
            elapsedTime = 0.f,
            timeScale = 1.f;
            UInt32 elapsedFrames = 0;
    };

    class Layer
    {
        friend class Application;

        Application *application;

    protected:
        Application &GetApplication() const { return *application; }
        Time & GetTime() const { return application->GetTime(); }

        explicit Layer(Application &app) :
            application(&app) {}

        virtual Int32 OnStart() = 0;
        virtual Int32 OnTick(const Time &time) = 0;
        virtual void OnStop(const Int32 exitCode) = 0;
        virtual Int32 OnTransitionFrom() { return EXIT_SUCCESS; }
        virtual Int32 OnTransitionTo() { return EXIT_SUCCESS; }
    };

    enum class State
    {
        Stopped = 0,
        Running = 1,
        Stopping = 2
    };

    struct Hooks
    {
        std::function<void()> preTick;
        std::function<void()> postTick;
        std::function<bool()> shouldContinue;
        std::function<void()> onStart;
        std::function<void()> onStop;
    };

private:
    State state = State::Stopped;
    Layer *runningLayer = nullptr;
    Time time{};

    Int32 OnStart(const Hooks &hooks);
    void OnStop(const Int32 exitCode, const Hooks &hooks);

public:
    Application(const Application &) = delete;
    Application &operator=(const Application &) = delete;
    Application(Application &&) = delete;
    Application &operator=(Application &&) = delete;

    Application() = default;
    ~Application() = default;

    [[nodiscard]] State GetState() const noexcept { return state; }
    [[nodiscard]] Time &GetTime() noexcept { return time; }

    template<typename TLayer, typename... TArgs>
    UInt32 SetRunningLayer(TArgs &&...arg)
    {
        static_assert(std::is_base_of_v<Layer, TLayer>,
            "Application::Run requires a layer type derived from ApplicationLayer.");

        static TLayer layer{*this, std::forward<TArgs>(arg)...}; // Application-wide for now.
        if (runningLayer == &layer)
            return EXIT_FAILURE;

        if (runningLayer != nullptr)
            runningLayer->OnTransitionFrom();

        runningLayer = &layer;
        runningLayer->OnTransitionTo();
        return EXIT_SUCCESS;
    }

    UInt32 Run(const Hooks &hooks = {});
    bool RequestStop();
};