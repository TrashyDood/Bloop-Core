#include "Shared.h"
#include "Transform.h"
#include "Camera.h"
#include "Mesh.h"
#include "Shader.h"
#include "Scene.h"

#include <glm/gtc/type_ptr.hpp>

#include <cstdlib>
#include <array>

struct MeshObject { Rendering::Mesh &mesh; Shared::Transform transform; };
struct MeshGeometry { Rendering::MeshVertices vertices; Rendering::MeshIndices indices; };

#pragma region Global Constants
const MeshGeometry CubeGeometry{
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
        0, 1, 2,
        0, 2, 3,
        5, 4, 7,
        5, 7, 6,
        1, 5, 6,
        1, 6, 2,
        4, 0, 3,
        4, 3, 7,
        3, 2, 6,
        3, 6, 7,
        4, 5, 1,
        4, 1, 0
    }};

const MeshGeometry PyramidGeometry{
    {
        {{-0.5f, 0.5f, 0.5f}, {}, {}, Colors::Red},
        {{0.5f, 0.5f, 0.5f}, {}, {}, Colors::Green},
        {{0.5f, 0.5f, -0.5f}, {}, {}, Colors::Blue},
        {{-0.5f, 0.5f, -0.5f}, {}, {}, Colors::Yellow},
        {{0.f, -0.5f, 0.f}, {}, {}, Colors::White},
    },
    {
        0, 1, 4,
        1, 2, 4,
        2, 3, 4,
        3, 0, 4,
        0, 3, 2,
        0, 2, 1
    }
};
#pragma endregion

class DemoScene : public Scene
{
    Rendering::Shader shader;
    Rendering::FlyingCamera camera;
    Rendering::Mesh cubeMesh{CubeGeometry.vertices, CubeGeometry.indices};
    Rendering::Mesh pyramidMesh{PyramidGeometry.vertices, PyramidGeometry.indices};

    std::array<MeshObject, 4> sceneObjects{{
        {cubeMesh, {{4.f, 0.f, 0.f}}},
        {cubeMesh, {{2.f, 0.f, 0.f}}},
        {pyramidMesh, {{0.f, 0.f, 0.f}}},
        {pyramidMesh, {{-2.f, 0.f, 0.f}}}
    }};

    GLint modelMatrixUniform,
        viewMatrixUniform,
        projectionMatrixUniform;

    double lastCursorX = 0.0, lastCursorY = 0.0;
    bool moveForward = false, moveBackward = false,
        moveLeft = false, moveRight = false,
        moveUp = false, moveDown = false,
        orbiting = false;

    bool wireframeEnabled = true;

#pragma region Input Handlers
    static void HandleKey(GLFWwindow *window, int key, int, int action, int)
    {
        DemoScene *scene = nullptr;
        if (!Shared::GetWindowUserPointer(*window, scene))
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

        if (key == GLFW_KEY_F && action == GLFW_PRESS)
            scene->camera.SetIsOrthographic(!scene->camera.IsOrthographic());

        if (key == GLFW_KEY_G && action == GLFW_PRESS)
            scene->wireframeEnabled = !scene->wireframeEnabled;
    }

    static void HandleMouseButton(GLFWwindow *window, int button, int action, int)
    {
        if (button != GLFW_MOUSE_BUTTON_1)
            return;

        DemoScene *scene = nullptr;
        if (!Shared::GetWindowUserPointer(*window, scene))
            return;

        scene->orbiting = action == GLFW_PRESS;
        if (scene->orbiting)
            glfwGetCursorPos(window, &scene->lastCursorX, &scene->lastCursorY);
    }

    static void HandleCursorPosition(GLFWwindow *window, double x, double y)
    {
        DemoScene *scene = nullptr;
        if (!Shared::GetWindowUserPointer(*window, scene))
            return;

        if (!scene->orbiting || glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1) != GLFW_PRESS)
        {
            scene->orbiting = false;
            return;
        }

        const float sensitivity = 0.005f,
            yawDelta = static_cast<float>(x - scene->lastCursorX) * sensitivity,
            pitchDelta = static_cast<float>(scene->lastCursorY - y) * sensitivity;

        scene->camera.Orbit(yawDelta, pitchDelta);
        scene->lastCursorX = x;
        scene->lastCursorY = y;
    }

    static void HandleScroll(GLFWwindow *window, double, double yOffset)
    {
        DemoScene *scene = nullptr;
        if (!Shared::GetWindowUserPointer(*window, scene))
            return;

        constexpr float zoomSensitivity = 0.1f;
        scene->camera.Zoom(yOffset * zoomSensitivity);
    }
#pragma endregion

    int InitializeShadersAndMeshes()
    {
        using namespace Rendering;

        shader = Shader({{"vertex.glsl", VertexShader, true},
                         {"fragment.glsl", FragmentShader, true}});

        if (!InitializeResources({&shader, &cubeMesh, &pyramidMesh}))
        {
            LOG_ERROR("Failed to initialize resources.\n");
            return EXIT_FAILURE;
        }

        if (!shader.GetUniformLocation("modelMatrix", modelMatrixUniform) ||
            !shader.GetUniformLocation("viewMatrix", viewMatrixUniform) ||
            !shader.GetUniformLocation("projectionMatrix", projectionMatrixUniform))
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

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);

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

        // Set the view and projection matrices based on the camera's current state.
        const Matrix4f viewMatrix = camera.GetViewMatrix();
        const Matrix4f projectionMatrix = camera.GetProjectionMatrix(ASPECT_RATIO(GetWindow()));
        glUniformMatrix4fv(viewMatrixUniform, 1, GL_FALSE, glm::value_ptr(viewMatrix));
        glUniformMatrix4fv(projectionMatrixUniform, 1, GL_FALSE, glm::value_ptr(projectionMatrix));


        const float pulseScale = 1.f + glm::sin(time.elapsedTime) * 0.5f;

        sceneObjects[0].transform.RotateEuler({0.f, 1.f * time.deltaTime, 0.f});
        sceneObjects[0].transform.Translate({0.f, glm::sin(time.elapsedTime) * time.deltaTime, 0.f});
        sceneObjects[0].transform.SetScale({pulseScale, pulseScale, pulseScale});

        sceneObjects[2].transform.RotateEuler({0.f, 1.f * time.deltaTime, 0.f});
        sceneObjects[2].transform.Translate({0.f, glm::sin(time.elapsedTime) * time.deltaTime, 0.f});
        sceneObjects[2].transform.SetScale({pulseScale, pulseScale, pulseScale});

        glPolygonMode(GL_FRONT_AND_BACK, wireframeEnabled ? GL_LINE : GL_FILL);

        for (auto& meshObject : sceneObjects)
        {
            const Matrix4f modelMatrix = meshObject.transform.GetTransformMatrix();
            glUniformMatrix4fv(modelMatrixUniform, 1, GL_FALSE, glm::value_ptr(modelMatrix));
            meshObject.mesh.Draw();
        }

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
        SceneConfig config{
            .targetTimeStep = 0.000001s,
            .enableVSync = true
        };
        result = scene.Start(*window, config);
    }

    if (window != nullptr)
        glfwDestroyWindow(window);

    glfwTerminate();

    return result;
}
