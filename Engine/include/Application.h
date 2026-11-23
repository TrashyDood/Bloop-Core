#pragma once

#include "Shared.h"

#include <functional>

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
    };

    class Layer
    {
        friend class Application;

        Application *application;

    protected:
        Application &GetApplication() const { return *application; }
        Time & GetTime() const { return application->GetTime(); }

        explicit Layer(Application &app) : application(&app) {}

        virtual UInt32 OnStart() = 0;
        virtual UInt32 OnTick(const Time &time) = 0;
        virtual void OnStop(const Int32 exitCode) = 0;
        virtual UInt32 OnTransitionFrom() { return EXIT_SUCCESS; }
        virtual UInt32 OnTransitionTo() { return EXIT_SUCCESS; }
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
    };

private:
    State state = State::Stopped;
    Layer *runningLayer = nullptr;
    Time time{};

    UInt32 SetActiveLayer(Layer *newLayer, Layer *&outPreviousLayer);

public:
    Application(const Application &) = delete;
    Application &operator=(const Application &) = delete;
    Application(Application &&) = delete;
    Application &operator=(Application &&) = delete;

    Application() = default;
    ~Application() = default;

    State GetState() const { return state; }
    Time &GetTime() { return time; }

    template<typename TLayer, typename... TArgs>
    void SetRunningLayer(TArgs &&...args)
    {
        static_assert(std::is_base_of_v<Layer, TLayer>,
            "Application::Run requires a layer type derived from ApplicationLayer.");

        static TLayer layer{*this, std::forward<TArgs>(args)...}; // Application-wide for now.
        SetActiveLayer(&layer);
    }

    UInt32 Run(const Hooks &hooks = {});
    void RequestStop();
    void OnStop();
};