#pragma once

#include "InputHandler.h"
#include "Shared.h"

#include <cstdlib>
#include <functional>

namespace Engine
{
    class Scene
    {
    public:
        struct SceneConfig
        {
            TimeDuration targetTimeStep = 0.0166667s;
            float timeScale = 1.0f;
        };

        struct SceneTime
        {
            float unscaledDeltaTime = 0.f,
                deltaTime = 0.f,
                unscaledElapsedTime = 0.f,
                elapsedTime = 0.f;
        };

        struct RunHooks
        {
            std::function<void()> preTick;
            std::function<void()> postTick;
            std::function<bool()> shouldContinue;
        };

        enum class State
        {
            Stopped = 0,
            Running = 1,
            Stopping = 2
        };

    private:
        State state = State::Stopped;
        SceneConfig *config = nullptr;

        SceneTime sceneTime;
        InputHandler inputHandler;

        int Start(SceneConfig &config);
        int Tick();
        void Stop(const int exitCode);

    protected:
        virtual int OnStart() = 0;
        virtual int OnMainTick(const SceneTime &time) = 0;
        virtual void OnCursorPosition(double, double) {}
        virtual void OnScroll(double, double) {}
        virtual void OnStop(const int exitCode) {}

    public:
        [[nodiscard]] const SceneTime &GetTime() const { return sceneTime; }
        [[nodiscard]] InputHandler &GetInput() { return inputHandler; }
        [[nodiscard]] State GetState() const { return state; }
        [[nodiscard]] SceneConfig &GetConfig() const { return *config; }
        [[nodiscard]] bool GetIsRunning() const { return GetState() == State::Running; }

        virtual ~Scene() = default;

        int Run(SceneConfig &config, const RunHooks &hooks = {});
        void RequestStop();

        void DispatchCursorPosition(const double x, const double y) { OnCursorPosition(x, y); }
        void DispatchScroll(const double xOffset, const double yOffset) { OnScroll(xOffset, yOffset); }
    };
}
