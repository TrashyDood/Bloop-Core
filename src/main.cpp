#include "Shared/Shared.h"
#include "Shared/Transform.h"
#include "Rendering/Camera.h"
#include "Rendering/Mesh.h"
#include "Rendering/Shader.h"
#include "Engine/Scene.h"

#include <glm/gtc/type_ptr.hpp>

#include <cstdlib>

const Rendering::MeshGeometry CubeGeometry{
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
    Rendering::Camera camera;
    Rendering::Shader shader;
    Rendering::Mesh cubeMesh;
    Shared::Transform cubeTransform;

    GLint modelLocation, viewLocation, projectionLocation;

    static void HandleKey(GLFWwindow *window, int key, int, int action, int)
    {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
            glfwSetWindowShouldClose(window, GLFW_TRUE);
    }

    int InitializeShadersAndMeshes()
    {
        using namespace Rendering;

        cubeMesh = Mesh(CubeGeometry);
        shader = Shader({{"vertex.glsl", VertexShader, true},
            {"fragment.glsl", FragmentShader, true}});

        if (!InitializeResources({&shader, &cubeMesh}))
        {
            LOG_ERROR("Failed to initialize resources.\n");
            return EXIT_FAILURE;
        }

        if (!shader.TryGetUniformLocation("modelMatrix", modelLocation) ||
            !shader.TryGetUniformLocation("viewMatrix", viewLocation) ||
            !shader.TryGetUniformLocation("projectionMatrix", projectionLocation))
        {
            LOG_ERROR("Transform or camera uniforms are missing from the shader program.\n");
            return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
    }

protected:
    int OnStart() override
    {
        if (InitializeShadersAndMeshes() != EXIT_SUCCESS)
            return EXIT_FAILURE;

        glfwSetKeyCallback(&GetWindow(), HandleKey);

        return EXIT_SUCCESS;
    }

    int OnUpdate(const SceneTime &) override { return EXIT_SUCCESS; }

    int OnDraw(const SceneTime &) override
    {
        shader.Use();

        const Matrix4f view = camera.GetViewMatrix();
        const Matrix4f projection = camera.GetProjectionMatrix(ASPECT_RATIO(GetWindow()));
        const Matrix4f model = cubeTransform.GetModelMatrix();

        glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(model));
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
        DemoScene scene;
        SceneConfig config;
        result = scene.Start(*window, config);
    }

    if (window != nullptr)
        glfwDestroyWindow(window);

    glfwTerminate();

    return result;
}
