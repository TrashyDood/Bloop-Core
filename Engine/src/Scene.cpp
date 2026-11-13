#include "Scene.h"
#include "ThreadPool.h"
#include "TickLoop.h"

namespace Engine
{
    int Scene::Run(Scene::SceneConfig &config, const Scene::SceneRunHooks &hooks)
    {
        int exitCode = Start(config);
        if (exitCode == EXIT_SUCCESS)
        {
            TickLoop tickLoop;
            tickLoop.Run([&](const TimeDuration &deltaTime, const TimeDuration &elapsedTime)
            {
                if (hooks.preTick) hooks.preTick();

                if (GetIsRunning())
                {
                    sceneTime.deltaTime = static_cast<float>(deltaTime.count());
                    sceneTime.elapsedTime = static_cast<float>(elapsedTime.count());
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

    int Scene::Start(Scene::SceneConfig &config)
    {
        if (state != State::Stopped)
            return EXIT_FAILURE;

        this->config = &config;
        state = State::Running;

        const int result = OnStart();
        if (result != EXIT_SUCCESS)
            state = State::Stopping;
        return result;
    }

    int Scene::Tick()
    {
        if (!GetIsRunning())
            return EXIT_FAILURE;

        const int exitCode = OnMainTick(sceneTime);

        if (exitCode != EXIT_SUCCESS)
            state = State::Stopping;

        return exitCode;
    }

    void Scene::Stop(const int exitCode)
    {
        if (state == State::Stopped)
            return;

        state = State::Stopped;
        OnStop(exitCode);
    }
}
