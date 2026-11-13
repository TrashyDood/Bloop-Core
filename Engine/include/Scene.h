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
            TimeDuration targetTimeStep = 0.0000001s;
        };

        struct SceneTime
        {
            float deltaTime = 0.0f;
            float elapsedTime = 0.0f;
        };

        struct SceneRunHooks
        {
            std::function<void()> preTick;
            std::function<void()> postTick;
            std::function<bool()> shouldContinue;
        };

        enum class State
        {
            Stopped,
            Running,
            Stopping
        };

    private:
        State state = State::Stopped;
        SceneConfig *config = nullptr;

        SceneTime sceneTime;
        InputHandler input;

        int Start(SceneConfig &config);
        int Tick();
        void Stop(const int exitCode);

    protected:
        [[nodiscard]] const SceneTime &GetSceneTime() const { return sceneTime; }
        [[nodiscard]] InputHandler &GetInput() { return input; }

        virtual int OnStart() = 0;
        virtual int OnMainTick(const SceneTime &time) = 0;
        virtual void OnCursorPosition(double, double) {}
        virtual void OnScroll(double, double) {}
        virtual void OnStop(const int exitCode) {}

    public:
        [[nodiscard]] State GetState() const { return state; }
        [[nodiscard]] SceneConfig &GetConfig() const { return *config; }
        [[nodiscard]] bool GetIsRunning() const { return GetState() == State::Running; }

        virtual ~Scene() = default;

        int Run(SceneConfig &config, const SceneRunHooks &hooks = {});
        void RequestStop()
        {
            if (state == State::Running)
                state = State::Stopping;
        }

        void DispatchKeyEvent(const KeyCode key, const KeyAction action)
        {
            input.OnKeyStateChanged(key, action);
        }

        void DispatchMouseButtonEvent(const MouseKeyCode button, const KeyAction action)
        {
            input.OnKeyStateChanged(button, action);
        }

        void DispatchCursorPosition(const double x, const double y) { OnCursorPosition(x, y); }
        void DispatchScroll(const double xOffset, const double yOffset) { OnScroll(xOffset, yOffset); }
    };
}
