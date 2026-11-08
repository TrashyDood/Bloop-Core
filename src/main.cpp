#include "Shared/Shared.h"
#include "Shared/Transform.h"
#include "Rendering/Camera.h"
#include "Rendering/Mesh.h"
#include "Rendering/Shader.h"
#include "Engine/Scene.h"

#include <glm/gtc/type_ptr.hpp>

#include <cstdlib>

const Rendering::MeshGeometry CubeGeometry {
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
    }};

class DemoScene : public Scene
{
    Rendering::Mesh mesh;
    Rendering::Shader shader;
    Rendering::FlyingCamera camera;
    Shared::Transform meshTransform;

    GLint modelMatrixUniform,
        viewMatrixUniform,
        projectionMatrixUniform;

    bool moveForward = false, moveBackward = false,
        moveLeft = false, moveRight = false,
        moveUp = false, moveDown = false,
        orbiting = false;

    double lastCursorX = 0.0, lastCursorY = 0.0;

private:
    static void HandleKey(GLFWwindow *window, int key, int, int action, int)
    {
        DemoScene *scene = nullptr;
        if (!Shared::TryGetWindowUserPointer(*window, scene))
            return;

        const bool pressed = action != GLFW_RELEASE;
        switch (key)
        {
            case GLFW_KEY_W: scene->moveForward = pressed; break;
            case GLFW_KEY_S: scene->moveBackward = pressed; break;
            case GLFW_KEY_A: scene->moveLeft = pressed; break;
            case GLFW_KEY_D: scene->moveRight = pressed; break;
            case GLFW_KEY_E: scene->moveUp = pressed; break;
            case GLFW_KEY_Q: scene->moveDown = pressed; break;
        }

        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
            glfwSetWindowShouldClose(window, GLFW_TRUE);

        if (key == GLFW_KEY_R && action == GLFW_PRESS)
        {
            scene->camera.ResetPosition();
            scene->moveForward = false;
            scene->moveBackward = false;
            scene->moveLeft = false;
            scene->moveRight = false;
            scene->moveUp = false;
            scene->moveDown = false;
            scene->orbiting = false;
        }
    }

    static void HandleMouseButton(GLFWwindow *window, int button, int action, int)
    {
        if (button != GLFW_MOUSE_BUTTON_1)
            return;

        DemoScene *scene = nullptr;
        if (!Shared::TryGetWindowUserPointer(*window, scene))
            return;

        scene->orbiting = action == GLFW_PRESS;
        if (scene->orbiting)
            glfwGetCursorPos(window, &scene->lastCursorX, &scene->lastCursorY);
    }

    static void HandleCursorPosition(GLFWwindow *window, double x, double y)
    {
        DemoScene *scene = nullptr;
        if (!Shared::TryGetWindowUserPointer(*window, scene))
            return;

        if (!scene->orbiting || glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1) != GLFW_PRESS)
        {
            scene->orbiting = false;
            return;
        }

        constexpr float sensitivity = 0.005f;
        const float yawDelta = static_cast<float>(x - scene->lastCursorX) * sensitivity;
        const float pitchDelta = static_cast<float>(scene->lastCursorY - y) * sensitivity;
        scene->camera.Orbit(yawDelta, pitchDelta);

        scene->lastCursorX = x;
        scene->lastCursorY = y;
    }

    static void HandleScroll(GLFWwindow *window, double, double yOffset)
    {
        DemoScene *scene = nullptr;
        if (!Shared::TryGetWindowUserPointer(*window, scene))
            return;

        constexpr float zoomSensitivity = 0.1f;
        scene->camera.Zoom(static_cast<float>(yOffset) * zoomSensitivity);
    }

    int InitializeShadersAndMeshes()
    {
        using namespace Rendering;

        mesh = Mesh(CubeGeometry);
        shader = Shader({{"vertex.glsl", VertexShader, true},
                         {"fragment.glsl", FragmentShader, true}});

        if (!InitializeResources({&shader, &mesh}))
        {
            LOG_ERROR("Failed to initialize resources.\n");
            return EXIT_FAILURE;
        }

        if (!shader.TryGetUniformLocation("modelMatrix", modelMatrixUniform) ||
            !shader.TryGetUniformLocation("viewMatrix", viewMatrixUniform) ||
            !shader.TryGetUniformLocation("projectionMatrix", projectionMatrixUniform))
        {
            LOG_ERROR("Transform or camera uniforms are missing from the shader program.\n");
            return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
    }

    int OnStart() override
    {
        if (InitializeShadersAndMeshes() != EXIT_SUCCESS)
            return EXIT_FAILURE;

        glfwSetKeyCallback(&GetWindow(), HandleKey);
        glfwSetMouseButtonCallback(&GetWindow(), HandleMouseButton);
        glfwSetCursorPosCallback(&GetWindow(), HandleCursorPosition);
        glfwSetScrollCallback(&GetWindow(), HandleScroll);

        camera.ResetPosition();

        return EXIT_SUCCESS;
    }

    int OnDraw(const SceneTime &time) override
    {
        camera.Move({
            ((float)(moveRight) - (float)(moveLeft)),
            ((float)(moveUp) - (float)(moveDown)),
            ((float)(moveForward) - (float)(moveBackward)),
        }, time.deltaTime);

        shader.Use();

        const Matrix4f viewMatrix = camera.GetViewMatrix();
        const Matrix4f projectionMatrix = camera.GetProjectionMatrix(ASPECT_RATIO(GetWindow()));
        const Matrix4f modelMatrix = meshTransform.GetModelMatrix();

        glUniformMatrix4fv(modelMatrixUniform, 1, GL_FALSE, glm::value_ptr(modelMatrix));
        glUniformMatrix4fv(viewMatrixUniform, 1, GL_FALSE, glm::value_ptr(viewMatrix));
        glUniformMatrix4fv(projectionMatrixUniform, 1, GL_FALSE, glm::value_ptr(projectionMatrix));

        mesh.Draw();

        return EXIT_SUCCESS;
    }
};

int main()
{
    GLFWwindow *window = nullptr;
    int result = Shared::SetupWindow(window, 800, 600, "Engine Window");

    if (result == EXIT_SUCCESS)
    {
        DemoScene scene;
        SceneConfig config;
        result = scene.Start(*window, config);
    }

    if (window != nullptr)
        glfwDestroyWindow(window);

    glfwTerminate();

    return result;
}
