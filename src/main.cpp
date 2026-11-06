#include "Shared/Core.h"
#include "Rendering/Camera.h"
#include "Rendering/Mesh.h"
#include "Rendering/Shader.h"

#include <glm/gtc/type_ptr.hpp>

#include <cstdio>
#include <cstdlib>

namespace
{
    void KeyCallback(GLFWwindow *window, int key, int, int action, int)
    {
    }

    // Gets called when the window is resized and updates the OpenGL viewport accordingly.
    void FramebufferSizeCallback(GLFWwindow *, int width, int height) { glViewport(0, 0, width, height); }

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
            glfwTerminate();
            return EXIT_FAILURE;
        }

        glfwMakeContextCurrent(outWindow);
        if (gladLoadGL(reinterpret_cast<GLADloadfunc>(glfwGetProcAddress)) == 0)
        {
            LOG_ERROR("Failed to load OpenGL functions with GLAD. Make sure that your graphics drivers are up to date and support OpenGL 3.3 or higher.\n");
            glfwDestroyWindow(outWindow);
            glfwTerminate();
            return EXIT_FAILURE;
        }

        int width = 0, height = 0;
        glfwSetFramebufferSizeCallback(outWindow, FramebufferSizeCallback);
        glfwGetFramebufferSize(outWindow, &width, &height);
        FramebufferSizeCallback(outWindow, width, height);
        glfwSwapInterval(1);
        return EXIT_SUCCESS;
    }
}

int main()
{
    GLFWwindow *window = nullptr;
    if (SetupWindow(window) != EXIT_SUCCESS)
        return EXIT_FAILURE;

    int result = EXIT_SUCCESS;
    {
        Rendering::Shader shader({
            {"vertex.glsl", Rendering::VertexShader, true},
            {"fragment.glsl", Rendering::FragmentShader, true},
        });
        Rendering::Mesh mesh(
            {
                {{-0.5f, -0.5f, 0.0f}, {}, {}, Colors::Red},
                {{0.5f, -0.5f, 0.0f}, {}, {}, Colors::Green},
                {{0.0f, 0.5f, 0.0f}, {}, {}, Colors::Blue},
            }, {0, 1, 2});

        if (!shader.InitializeResources() || !mesh.InitializeResources())
            result = EXIT_FAILURE;
        else
        {
            // glfwSetWindowUserPointer(window, &house);
            glfwSetKeyCallback(window, KeyCallback);
            Rendering::Camera camera;

            double lastMouseX = 0.0;
            double lastMouseY = 0.0;
            bool orbiting = false;

            const GLint viewLocation = glGetUniformLocation(shader.GetProgramHandle(), "view");
            const GLint projectionLocation = glGetUniformLocation(shader.GetProgramHandle(), "projection");
            if (viewLocation < 0 || projectionLocation < 0)
            {
                LOG_ERROR("Camera uniforms are missing from the shader program.\n");
                result = EXIT_FAILURE;
            }

            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glEnable(GL_DEPTH_TEST);

            while (result == EXIT_SUCCESS && glfwWindowShouldClose(window) == GLFW_FALSE)
            {
                glfwPollEvents();

                const bool mouseDown = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1) == GLFW_PRESS;
                if (mouseDown)
                {
                    double mouseX = 0.0;
                    double mouseY = 0.0;
                    glfwGetCursorPos(window, &mouseX, &mouseY);
                    if (orbiting)
                    {
                        camera.ProcessMouseMovement(
                            static_cast<float>(mouseX - lastMouseX),
                            static_cast<float>(lastMouseY - mouseY));
                    }
                    lastMouseX = mouseX;
                    lastMouseY = mouseY;
                }
                orbiting = mouseDown;

                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                shader.Use();
                const Matrix4f view = camera.GetViewMatrix();

                int framebufferWidth = 0;
                int framebufferHeight = 0;
                glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
                if (framebufferWidth > 0 && framebufferHeight > 0)
                {
                    const float aspectRatio =
                        static_cast<float>(framebufferWidth) / static_cast<float>(framebufferHeight);
                    const Matrix4f& projection = camera.GetProjectionMatrix(aspectRatio);
                    glUniformMatrix4fv(viewLocation, 1, GL_FALSE, glm::value_ptr(view));
                    glUniformMatrix4fv(projectionLocation, 1, GL_FALSE, glm::value_ptr(projection));
                    mesh.Draw();
                }

                CHECK_GL_ERRORS();       // Check for any OpenGL errors that may have occurred during rendering.
                glfwSwapBuffers(window); // Swap the front(read) and back(write) buffers to display the rendered image.
            }
        }
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return result;
}
