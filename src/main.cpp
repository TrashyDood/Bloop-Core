#include "Shared/Shared.h"
#include "Rendering/Camera.h"
#include "Rendering/Mesh.h"
#include "Rendering/Shader.h"

#include <glm/gtc/type_ptr.hpp>

#include <cstdlib>

namespace
{
    void KeyCallback(GLFWwindow *window, int key, int, int action, int)
    {
    }

    void FramebufferSizeCallback(GLFWwindow *, int width, int height) { glViewport(0, 0, width, height); }

    float GetAspectRatio(GLFWwindow &window)
    {
        int width = 0, height = 0;
        glfwGetFramebufferSize(&window, &width, &height);
        return static_cast<float>(width) / static_cast<float>(height);
    }

    int SetupWindow(GLFWwindow *&outWindow)
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

        int width = 0, height = 0;
        glfwSetFramebufferSizeCallback(outWindow, FramebufferSizeCallback);
        glfwGetFramebufferSize(outWindow, &width, &height);
        FramebufferSizeCallback(outWindow, width, height);
        glfwSetKeyCallback(outWindow, KeyCallback);
        glfwSwapInterval(1);

        return EXIT_SUCCESS;
    }

    int Draw(GLFWwindow &window, Rendering::Camera &camera)
    {
        using namespace Rendering;

        Shader shader({{"vertex.glsl", VertexShader, true},
                       {"fragment.glsl", FragmentShader, true}});

        Mesh triangleMesh(
            {{{-0.5f, -0.5f, 0.0f}, {}, {}, Colors::Red},
             {{0.5f, -0.5f, 0.0f}, {}, {}, Colors::Green},
             {{0.0f, 0.5f, 0.0f}, {}, {}, Colors::Blue}},
            {0, 1, 2});

        if (!InitializeResources({&shader, &triangleMesh}))
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

        while (glfwWindowShouldClose(&window) == GLFW_FALSE)
        {
            glfwPollEvents();
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            shader.Use();

            const Matrix4f view = camera.GetViewMatrix();
            const Matrix4f projection = camera.GetProjectionMatrix(GetAspectRatio(window));

            glUniformMatrix4fv(viewLocation, 1, GL_FALSE, glm::value_ptr(view));
            glUniformMatrix4fv(projectionLocation, 1, GL_FALSE, glm::value_ptr(projection));

            triangleMesh.Draw();

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
    GLFWwindow *window = nullptr;
    int result = EXIT_SUCCESS;

    if (result == EXIT_SUCCESS)
        result = SetupWindow(window);

    if (result == EXIT_SUCCESS)
        result = Draw(*window, camera);

    if (window != nullptr)
        glfwDestroyWindow(window);
    glfwTerminate();

    return result;
}
