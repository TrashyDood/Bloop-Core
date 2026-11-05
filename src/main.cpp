#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include "Shared/Utils.h"
#include "Rendering/ShaderUtils.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>

namespace
{
    bool TryCreateHouseShaderProgram(GLuint &outShaderProgram)
    {
        using namespace ShaderUtils;

        return ShaderUtils::CreateShaderProgram(
            outShaderProgram,
            ShaderSource{"house_vertex.glsl", ShaderType::Vertex, true},
            ShaderSource{"house_fragment.glsl", ShaderType::Fragment, true}) == ShaderReturnCode::None;
    }

    template <typename T>
    bool TryGetGLWindowUserPointer(GLFWwindow *window, T *&outPointer)
    {
        outPointer = static_cast<T *>(glfwGetWindowUserPointer(window));
        return outPointer != nullptr;
    }

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
            std::fputs("Failed to load OpenGL functions with GLAD.\n", stderr);
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

    GLuint shaderProgram = 0;
    if (!TryCreateHouseShaderProgram(shaderProgram))
    {
        glfwDestroyWindow(window);
        glfwTerminate();
        return EXIT_FAILURE;
    }

    //glfwSetWindowUserPointer(window, &house);
    glfwSetKeyCallback(window, KeyCallback);

    // Define the vertices of a triangle in normalized device coordinates.
    constexpr float triangleShape[] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f};

    GLuint vao = 0, vbo = 0;
    glGenVertexArrays(1, &vao);                                                          // Create a Vertex Array Object to store the vertex layout.
    glGenBuffers(1, &vbo);                                                               // Create a Vertex Buffer Object to hold the triangle's vertex data.
    glBindVertexArray(vao);                                                              // Make this VAO active for setup context.
    glBindBuffer(GL_ARRAY_BUFFER, vbo);                                                  // Select the VBO as the vertex data buffer for active VAO.
    glBufferData(GL_ARRAY_BUFFER, sizeof(triangleShape), triangleShape, GL_STATIC_DRAW); // Upload the vertices to the GPU.
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);         // Describe attribute 0 as 2 floats per vertex.
    glEnableVertexAttribArray(0);                                                        // Enable the vertex position attribute.

    glClearColor(0.1f, 0.1f, 0.1f, 1.0f); // Set the clear color for the window to a dark gray.

    while (glfwWindowShouldClose(window) == GLFW_FALSE) // Run loop until the user closes the window.
    {
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(shaderProgram);
        glBindVertexArray(vao);

        Utils::CheckOpenGLErrors(); // Check for any OpenGL errors that may have occurred during rendering.
        glfwSwapBuffers(window);    // Swap the front(read) and back(write) buffers to display the rendered image.
        glfwPollEvents();           // Poll for events such as keyboard and mouse input.
    }

    // Cleanup resources before exiting.
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(shaderProgram);
    glfwDestroyWindow(window);
    glfwTerminate();
    return EXIT_SUCCESS;
}
