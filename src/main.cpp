#include "Shared/Shared.h"
#include "Rendering/Camera.h"
#include "Rendering/Mesh.h"
#include "Rendering/Shader.h"
#include "Engine/Scene.h"

#include <glm/gtc/type_ptr.hpp>

#include <cstdlib>

class CustomScene : public Scene
{
    Rendering::Camera camera;
    Rendering::Shader shader;
    Rendering::Mesh cubeMesh;

    GLint viewLocation, projectionLocation;
    bool moveForward = false;
    bool moveBackward = false;
    bool moveLeft = false;
    bool moveRight = false;
    bool orbiting = false;
    double lastCursorX = 0.0;
    double lastCursorY = 0.0;

    static void HandleKey(GLFWwindow *window, int key, int, int action, int)
    {
        CustomScene *scene = nullptr;
        if (!Rendering::TryGetWindowUserPointer(*window, scene))
            return;

        const bool pressed = action != GLFW_RELEASE;
        switch (key)
        {
        case GLFW_KEY_W: scene->moveForward = pressed; break;
        case GLFW_KEY_S: scene->moveBackward = pressed; break;
        case GLFW_KEY_A: scene->moveLeft = pressed; break;
        case GLFW_KEY_D: scene->moveRight = pressed; break;
        case GLFW_KEY_ESCAPE:
            if (action == GLFW_PRESS)
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            break;
        }
    }

    static void HandleCursorPosition(GLFWwindow *window, double x, double y)
    {
        CustomScene *scene = nullptr;
        if (!Rendering::TryGetWindowUserPointer(*window, scene))
            return;

        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1) != GLFW_PRESS)
        {
            scene->orbiting = false;
            return;
        }

        if (scene->orbiting)
            scene->camera.Rotate(
                static_cast<float>(x - scene->lastCursorX) * 0.1f,
                static_cast<float>(y - scene->lastCursorY) * 0.1f);

        scene->lastCursorX = x;
        scene->lastCursorY = y;
        scene->orbiting = true;
    }

    static void HandleScroll(GLFWwindow *window, double, double yOffset)
    {
        CustomScene *scene = nullptr;
        if (Rendering::TryGetWindowUserPointer(*window, scene))
            scene->camera.Zoom(static_cast<float>(yOffset));
    }

    int InitializeShadersAndMeshes()
    {
        using namespace Rendering;

        shader = Shader({{"vertex.glsl", VertexShader, true},
            {"fragment.glsl", FragmentShader, true}});

        cubeMesh = Mesh(
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

        viewLocation = shader.GetUniformLocation("view");
        projectionLocation = shader.GetUniformLocation("projection");
        if (viewLocation < 0 || projectionLocation < 0)
        {
            LOG_ERROR("Camera uniforms are missing from the shader program.\n");
            return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
    }

protected:
    int OnStart() override
    {
        using namespace Rendering;

        if (InitializeShadersAndMeshes() != EXIT_SUCCESS)
            return EXIT_FAILURE;

        glfwSetKeyCallback(&GetWindow(), HandleKey);
        glfwSetCursorPosCallback(&GetWindow(), HandleCursorPosition);
        glfwSetScrollCallback(&GetWindow(), HandleScroll);

        return EXIT_SUCCESS;
    }

    int OnUpdate(const SceneTime &time) override { return EXIT_SUCCESS; }

    int OnDraw(const SceneTime &time) override
    {
        constexpr float cameraSpeed = 2.5f;
        const float forward = static_cast<float>(moveForward) - static_cast<float>(moveBackward);
        const float right = static_cast<float>(moveRight) - static_cast<float>(moveLeft);
        camera.Move(forward * cameraSpeed * time.deltaTime, right * cameraSpeed * time.deltaTime);

        shader.Use();

        const Matrix4f view = camera.GetViewMatrix();
        const Matrix4f projection = camera.GetProjectionMatrix(ASPECT_RATIO(GetWindow()));

        glUniformMatrix4fv(viewLocation, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projectionLocation, 1, GL_FALSE, glm::value_ptr(projection));

        cubeMesh.Draw();

        return EXIT_SUCCESS;
    }

    void OnStop(const int exitCode) override { }
};

int main()
{
    GLFWwindow *window = nullptr;
    int result = Rendering::SetupWindow(window, 800, 600, "Engine Window");

    if (result == EXIT_SUCCESS)
    {
        CustomScene scene;
        SceneConfig config{0.016667s, true};
        result = scene.Start(*window, config);
    }

    if (window != nullptr)
        glfwDestroyWindow(window);

    glfwTerminate();

    return result;
}
