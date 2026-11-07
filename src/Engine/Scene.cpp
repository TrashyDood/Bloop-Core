#include "Engine/Scene.h"

#include "Rendering/Rendering.h"
#include "Shared/TickLoop.h"
#include "Shared/ThreadPool.h"

int Scene::Start(GLFWwindow *window, SceneConfig *config)
{
    if (isRunning)
        return EXIT_FAILURE;

    glfwSetWindowUserPointer(window, dynamic_cast<void *>(this));
    isRunning = true;
    int result = OnStart();

    if (result == EXIT_SUCCESS)
    {
        glfwSetWindowUserPointer(&GetWindow(), dynamic_cast<void *>(this));
        glfwSwapInterval(config->enableVSync ? 1 : 0);
        glfwSetFramebufferSizeCallback(&GetWindow(), [](GLFWwindow *, int width, int height) { glViewport(0, 0, width, height); });

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glEnable(GL_DEPTH_TEST);

        TickLoop tickLoop;
        tickLoop.Run([&](float deltaTime, float elapsedTime)
        {
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glfwPollEvents();

            SceneTime sceneTime;
            sceneTime.deltaTime = deltaTime;
            sceneTime.elapsedTime = elapsedTime;

            ThreadTask<int> updateTask =
                StaticThreadPool().SubmitTask([&]() { return OnUpdate(sceneTime); });

            int drawResult = OnDraw(sceneTime);
            int updateResult = updateTask.get();
            result = drawResult == EXIT_SUCCESS ? updateResult : drawResult;

            CHECK_GL_ERRORS();
            glfwSwapBuffers(window);

            return !glfwWindowShouldClose(window) && isRunning && result == EXIT_SUCCESS;
        },
        config->targetTimeStep);
    }

    isRunning = false;
    OnStop(result);
    return result;
}

int Scene::Start(GLFWwindow &window, SceneConfig &config)
{
    if (isRunning)
        return EXIT_FAILURE;

    Scene::config = &config;
    Scene::window = &window;

    return Start(Scene::window, Scene::config);
}
