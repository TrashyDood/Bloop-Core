#include "Scene.h"
#include "TickLoop.h"
#include "ThreadPool.h"

void Scene::HandleKeyEvent(GLFWwindow *window, int key, int, int action, int)
{
    Scene *scene = nullptr;
    if (!Shared::GetWindowUserPointer(*window, scene))
        return;

    scene->input.OnKeyStateChanged(
        static_cast<Engine::KeyCode>(key),
        static_cast<Engine::KeyAction>(action));
}

void Scene::HandleMouseButtonEvent(GLFWwindow *window, int button, int action, int)
{
    Scene *scene = nullptr;
    if (!Shared::GetWindowUserPointer(*window, scene))
        return;

    scene->input.OnKeyStateChanged(
        static_cast<Engine::MouseKeyCode>(button),
        static_cast<Engine::KeyAction>(action));
}

int Scene::Start(GLFWwindow *window, SceneConfig *config)
{
    Scene::config = config;
    Scene::window = window;
    glfwSetWindowUserPointer(window, dynamic_cast<void *>(this));
    isRunning = true;
    int result = OnStart();
    
    if (result == EXIT_SUCCESS)
    {
        glfwSwapInterval(config->enableVSync ? 1 : 0);
        glfwSetKeyCallback(&GetWindow(), HandleKeyEvent);
        glfwSetMouseButtonCallback(&GetWindow(), HandleMouseButtonEvent);
        glfwSetFramebufferSizeCallback(&GetWindow(), [](GLFWwindow *, int width, int height)
        {
            glViewport(0, 0, width, height);
        });

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

        TickLoop tickLoop;
        ThreadTask<int> updateTask;
        tickLoop.Run([&](const TimeDuration &deltaTime, const TimeDuration &elapsedTime)
        {
            glfwPollEvents();

            sceneTime.deltaTime = std::chrono::duration<float>(deltaTime).count();
            sceneTime.elapsedTime = std::chrono::duration<float>(elapsedTime).count();

            updateTask =
                StaticThreadPool().SubmitTask([&]() { return OnUpdate(sceneTime); });

            int drawResult = OnDraw(sceneTime);
            int updateResult = updateTask.get();
            result = drawResult == EXIT_SUCCESS ? updateResult : drawResult;

            CHECK_GL_ERRORS();
            
            glfwSwapBuffers(window);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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

    return Start(&window, &config);
}
