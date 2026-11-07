#pragma once

#include "Shared/Shared.h"
#include "Shared/TickLoop.h"
#include "Shared/ThreadPool.h"

#include "Rendering/Rendering.h"

class Scene
{
    bool isRunning = false;

protected:
    Scene() = default;

    virtual int OnStart() = 0;
    virtual int OnUpdate(const float deltaTime, const float elapsedTime) = 0;
    virtual int OnDraw(const float deltaTime, const float elapsedTime) = 0;
    virtual void OnStop(const int exitCode) = 0;

public:
    virtual ~Scene() = default;

    int Start(std::chrono::duration<float> targetTimeStep = 0.016667s)
    {
        if (isRunning)
            return EXIT_FAILURE;

        isRunning = true;
        int result = OnStart();

        if (result == EXIT_SUCCESS)
        {
            TickLoop tickLoop;
            tickLoop.Run([&](float deltaTime, float elapsedTime)
            {
                ThreadTask<int> updateTask =
                    StaticThreadPool().SubmitTask([&]() { return OnUpdate(deltaTime, elapsedTime); });

                int drawResult = OnDraw(deltaTime, elapsedTime);
                int updateResult = updateTask.get();
                result = drawResult == EXIT_SUCCESS ? updateResult : drawResult;

                return isRunning && result == EXIT_SUCCESS;
            }, targetTimeStep);
        }

        isRunning = false;
        OnStop(result);
        return result;
    }
};