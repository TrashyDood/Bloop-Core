#include "Shared/Shared.h"
#include "Rendering/Camera.h"
#include "Rendering/Mesh.h"
#include "Rendering/Shader.h"

#include <glm/gtc/type_ptr.hpp>

#include <cstdlib>

namespace
{
    struct InputState
    {
        Rendering::Camera *camera = nullptr;
        double lastCursorX = 0.0;
        double lastCursorY = 0.0;
        bool firstCursorEvent = true;
    };

    void KeyCallback(GLFWwindow *window, int key, int, int action, int)
    {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
            glfwSetWindowShouldClose(window, GLFW_TRUE);
    }

    void CursorPositionCallback(GLFWwindow *window, double x, double y)
    {
        auto *input = static_cast<InputState *>(glfwGetWindowUserPointer(window));
        if (input == nullptr || input->camera == nullptr)
            return;

        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) != GLFW_PRESS)
        {
            input->firstCursorEvent = true;
            return;
        }

        if (input->firstCursorEvent)
        {
            input->lastCursorX = x;
            input->lastCursorY = y;
            input->firstCursorEvent = false;
            return;
        }

        const float xOffset = static_cast<float>(x - input->lastCursorX);
        const float yOffset = static_cast<float>(input->lastCursorY - y);
        input->lastCursorX = x;
        input->lastCursorY = y;
        input->camera->Rotate(xOffset * 0.1f, yOffset * 0.1f);
    }

    void ScrollCallback(GLFWwindow *window, double, double yOffset)
    {
        auto *input = static_cast<InputState *>(glfwGetWindowUserPointer(window));
        if (input != nullptr && input->camera != nullptr)
            input->camera->Zoom(static_cast<float>(yOffset) * 0.5f);
    }

    void FramebufferSizeCallback(GLFWwindow *, int width, int height) { glViewport(0, 0, width, height); }

    float GetWindowAspectRatio(GLFWwindow &window)
    {
        int width = 0, height = 0;
        glfwGetFramebufferSize(&window, &width, &height);
        return static_cast<float>(width) / static_cast<float>(height);
    }

    int SetupWindow(GLFWwindow *&outWindow, InputState &input)
    {
        if (glfwInit() != GLFW_TRUE)
            return EXIT_FAILURE;

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        outWindow = glfwCreateWindow(1280, 720, "Engine", nullptr, nullptr);
        if (outWindow == nullptr)
        {
            LOG_ERROR("Failed to create GLFW window. Make sure that your graphics drivers are up to date and support OpenGL 3.3 or higher.\n");
            return EXIT_FAILURE;
        }

        glfwMakeContextCurrent(outWindow);
        if (gladLoadGL(reinterpret_cast<GLADloadfunc>(glfwGetProcAddress)) == 0)
        {
            LOG_ERROR("Failed to load OpenGL functions with GLAD. Make sure that your graphics drivers are up to date and support OpenGL 3.3 or higher.\n");
            return EXIT_FAILURE;
        }

        glfwSetKeyCallback(outWindow, KeyCallback);
        glfwSetCursorPosCallback(outWindow, CursorPositionCallback);
        glfwSetScrollCallback(outWindow, ScrollCallback);

        glfwSetWindowUserPointer(outWindow, &input);

        int width = 0, height = 0;
        glfwGetFramebufferSize(outWindow, &width, &height);
        FramebufferSizeCallback(outWindow, width, height);
        glfwSetFramebufferSizeCallback(outWindow, FramebufferSizeCallback);

        glfwSwapInterval(1);

        return EXIT_SUCCESS;
    }

    int Draw(GLFWwindow &window, Rendering::Camera &camera)
    {
        using namespace Rendering;

        Shader shader({{"vertex.glsl", VertexShader, true},
                       {"fragment.glsl", FragmentShader, true}});

        Mesh cubeMesh(
            {
                {{-0.5f, -0.5f, 0.5f}, {}, {}, Colors::Red},
                {{0.5f, -0.5f, 0.5f}, {}, {}, Colors::Green},
                {{0.5f, 0.5f, 0.5f}, {}, {}, Colors::Blue},
                {{-0.5f, 0.5f, 0.5f}, {}, {}, Colors::Yellow},
                {{-0.5f, -0.5f, -0.5f}, {}, {}, Colors::Cyan},
                {{0.5f, -0.5f, -0.5f}, {}, {}, Colors::Magenta},
                {{0.5f, 0.5f, -0.5f}, {}, {}, Colors::Orange},
                {{-0.5f, 0.5f, -0.5f}, {}, {}, Colors::Purple},
            },
            {
                0, 1, 2, 0, 2, 3,
                5, 4, 7, 5, 7, 6,
                1, 5, 6, 1, 6, 2,
                4, 0, 3, 4, 3, 7,
                3, 2, 6, 3, 6, 7,
                4, 5, 1, 4, 1, 0,
            });

        if (!InitializeResources({&shader, &cubeMesh}))
        {
            LOG_ERROR("Failed to initialize resources.\n");
            return EXIT_FAILURE;
        }

        const GLint viewLocation = shader.GetUniformLocation("view");
        const GLint projectionLocation = shader.GetUniformLocation("projection");
        if (viewLocation < 0 || projectionLocation < 0)
        {
            LOG_ERROR("Camera uniforms are missing from the shader program.\n");
            return EXIT_FAILURE;
        }

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glEnable(GL_DEPTH_TEST);

        double previousFrameTime = glfwGetTime();
        while (glfwWindowShouldClose(&window) == GLFW_FALSE)
        {
            glfwPollEvents();

            const double currentFrameTime = glfwGetTime();
            const float deltaTime = static_cast<float>(currentFrameTime - previousFrameTime);
            previousFrameTime = currentFrameTime;

            const float forwardOffset =
                static_cast<float>(glfwGetKey(&window, GLFW_KEY_W) == GLFW_PRESS) -
                static_cast<float>(glfwGetKey(&window, GLFW_KEY_S) == GLFW_PRESS);
            const float rightOffset =
                static_cast<float>(glfwGetKey(&window, GLFW_KEY_D) == GLFW_PRESS) -
                static_cast<float>(glfwGetKey(&window, GLFW_KEY_A) == GLFW_PRESS);
            constexpr float cameraSpeed = 2.5f;
            camera.Move(forwardOffset * cameraSpeed * deltaTime,
                        rightOffset * cameraSpeed * deltaTime);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            shader.Use();

            const Matrix4f view = camera.GetViewMatrix();
            const Matrix4f projection = camera.GetProjectionMatrix(GetWindowAspectRatio(window));

            glUniformMatrix4fv(viewLocation, 1, GL_FALSE, glm::value_ptr(view));
            glUniformMatrix4fv(projectionLocation, 1, GL_FALSE, glm::value_ptr(projection));

            cubeMesh.Draw();

            CHECK_GL_ERRORS();
            glfwSwapBuffers(&window);
        }

        return EXIT_SUCCESS;
    }
}

int main()
{
    using namespace Rendering;

    Camera camera;
    InputState input{&camera};
    GLFWwindow *window = nullptr;
    int result = EXIT_SUCCESS;

    if (result == EXIT_SUCCESS)
        result = SetupWindow(window, input);

    if (result == EXIT_SUCCESS)
        result = Draw(*window, camera);

    if (window != nullptr)
        glfwDestroyWindow(window);
    glfwTerminate();

    return result;
}
