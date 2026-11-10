#include "Shared.h"

#include <glad/gl.h>

#include <cstdarg>
#include <cstdio>

void Utils::CheckOpenGLErrors()
{
    for (GLenum error = glGetError(); error != GL_NO_ERROR; error = glGetError())
        std::fprintf(stderr, "OpenGL error: 0x%X\n", error);
}

void Utils::WriteError(const char *msg, ...)
{
    va_list args;
    va_start(args, msg);
    std::vfprintf(stderr, msg, args);
    va_end(args);
}

int Shared::SetupWindow(GLFWwindow *&outWindow, const int width, const int height, const char *title)
{
    if (glfwInit() != GLFW_TRUE)
        return EXIT_FAILURE;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    outWindow = glfwCreateWindow(width, height, title, nullptr, nullptr);
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

    return EXIT_SUCCESS;
}

float Shared::GetWindowAspectRatio(GLFWwindow &window)
{
    int width = 0, height = 0;
    glfwGetFramebufferSize(&window, &width, &height);
    return static_cast<float>(glm::max(width, 1)) / static_cast<float>(glm::max(height, 1));
}