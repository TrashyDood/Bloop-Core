#include "Scene.h"
#include "ThreadPool.h"
#include "TickLoop.h"
#include "LogHandler.h"

namespace Engine
{
    Int32 Scene::Run(Scene::SceneConfig &config, const Scene::RunHooks &hooks)
    {
        Int32 exitCode = Start(config);
        if (exitCode == EXIT_SUCCESS)
        {
            TickLoop tickLoop;
            tickLoop.Run([&](const TimeDuration &deltaTime, const TimeDuration &elapsedTime)
            {
                if (hooks.preTick) hooks.preTick();

                if (GetIsRunning())
                {
                    sceneTime.unscaledDeltaTime = static_cast<float>(deltaTime.count());
                    sceneTime.deltaTime = sceneTime.unscaledDeltaTime * config.timeScale;
                    sceneTime.unscaledElapsedTime += sceneTime.unscaledDeltaTime;
                    sceneTime.elapsedTime += sceneTime.deltaTime;

                    exitCode = Tick();
                }

                if (hooks.postTick) hooks.postTick();

                return GetIsRunning()
                    && exitCode == EXIT_SUCCESS
                    && (!hooks.shouldContinue || hooks.shouldContinue());
            }, config.targetTimeStep);
        }

        Stop(exitCode);
        return exitCode;
    }

    void Scene::RequestStop()
    {
        if (state == State::Running)
        {
            LOG_INFO("Scene stop requested.\n");
            state = State::Stopping;
        }
    }

    Int32 Scene::Start(Scene::SceneConfig &config)
    {
        if (state != State::Stopped)
        {
            LOG_ERROR("Cannot start scene: it is not stopped.\n");
            return EXIT_FAILURE;
        }

        this->config = &config;
        state = State::Running;

        const Int32 result = OnStart();
        if (result != EXIT_SUCCESS)
        {
            LOG_ERROR("Scene startup failed with exit code %d.\n", result);
            state = State::Stopping;
        }
        return result;
    }

    Int32 Scene::Tick()
    {
        if (!GetIsRunning())
            return EXIT_FAILURE;

        const Int32 exitCode = OnMainTick(sceneTime);

        if (exitCode != EXIT_SUCCESS)
        {
            LOG_ERROR("Scene tick failed with exit code %d.\n", exitCode);
            state = State::Stopping;
        }

        return exitCode;
    }

    void Scene::Stop(const Int32 exitCode)
    {
        if (state == State::Stopped)
            return;

        state = State::Stopped;
        OnStop(exitCode);
    }
}
