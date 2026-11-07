#pragma once

#include "Shared/Shared.h"
#include "Shared/TickLoop.h"
#include "Shared/ThreadPool.h"

#include "Rendering/Rendering.h"

struct SceneConfig
{
    const std::chrono::duration<float> targetTimeStep = 0.016667s;
    const bool enableVSync = true;
};

class Scene
{
    bool isRunning = false;
    GLFWwindow &window;

protected:
    [[nodiscard]] GLFWwindow &GetWindow() const { return window; }

    explicit Scene(GLFWwindow &window) : window(window) {}

    virtual int OnStart() = 0;
    virtual int OnUpdate(const float deltaTime, const float elapsedTime) = 0;
    virtual int OnDraw(const float deltaTime, const float elapsedTime) = 0;
    virtual void OnStop(const int exitCode) = 0;

public:
    virtual ~Scene() = default;

    int Start(const SceneConfig &config = {})
    {
        if (isRunning)
            return EXIT_FAILURE;

        isRunning = true;
        int result = OnStart();

        if (result == EXIT_SUCCESS)
        {
            glfwSwapInterval(config.enableVSync ? 1 : 0); // Enable V-Sync
            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glEnable(GL_DEPTH_TEST);

            TickLoop tickLoop;
            tickLoop.Run([&](float deltaTime, float elapsedTime)
            {
                glfwPollEvents();
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                ThreadTask<int> updateTask =
                    StaticThreadPool().SubmitTask([&]() { return OnUpdate(deltaTime, elapsedTime); });

                int drawResult = OnDraw(deltaTime, elapsedTime);
                int updateResult = updateTask.get();
                result = drawResult == EXIT_SUCCESS ? updateResult : drawResult;

                CHECK_GL_ERRORS();
                glfwSwapBuffers(&window);

                return !glfwWindowShouldClose(&window) && isRunning && result == EXIT_SUCCESS;
            },
            config.targetTimeStep);
        }

        isRunning = false;
        OnStop(result);
        return result;
    }
};