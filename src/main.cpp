#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include "House2D.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>

namespace
{
    // Load shader source code from a file into a string.
    bool LoadShaderSource(const char *filename, std::string &source)
    {
        const std::string sourcePath = std::string(ENGINE_SHADER_DIR) + "/" + filename;
        std::ifstream file(sourcePath);
        if (!file)
        {
            std::fprintf(stderr, "Failed to open shader file: %s\n", sourcePath.c_str());
            return false;
        }

        std::ostringstream contents;
        contents << file.rdbuf();
        if (file.bad())
        {
            std::fprintf(stderr, "Failed to read shader file: %s\n", sourcePath.c_str());
            return false;
        }

        source = contents.str();
        return true;
    }

    // Compile a shader of the given type (vertex or fragment) from source code.
    GLuint CompileShader(GLenum type, const char *source)
    {
        const GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &source, nullptr);
        glCompileShader(shader);

        GLint success = GL_FALSE;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (success == GL_FALSE)
        {
            char log[512]{};
            glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
            std::fprintf(stderr, "Shader compilation failed: %s\n", log);
            glDeleteShader(shader);
            return 0;
        }

        return shader;
    }

    // Create a shader program by compiling and linking vertex and fragment shaders.
    GLuint CreateShaderProgram()
    {
        std::string vertex_source;
        std::string fragment_source;
        if (!LoadShaderSource("vertex.glsl", vertex_source) ||
            !LoadShaderSource("fragment.glsl", fragment_source))
            return 0;

        const GLuint vertex_shader = CompileShader(GL_VERTEX_SHADER, vertex_source.c_str());
        const GLuint fragment_shader = CompileShader(GL_FRAGMENT_SHADER, fragment_source.c_str());
        if (vertex_shader == 0 || fragment_shader == 0)
        {
            glDeleteShader(vertex_shader);
            glDeleteShader(fragment_shader);
            return 0;
        }

        const GLuint shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vertex_shader);
        glAttachShader(shaderProgram, fragment_shader);
        glLinkProgram(shaderProgram);
        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);

        GLint success = GL_FALSE;
        glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
        if (success == GL_FALSE)
        {
            char log[512]{};
            glGetProgramInfoLog(shaderProgram, sizeof(log), nullptr, log);
            std::fprintf(stderr, "Shader link failed: %s\n", log);
            glDeleteProgram(shaderProgram);
            return 0;
        }

        return shaderProgram;
    }

    template <typename T>
    bool TryGetGLWindowUserPointer(GLFWwindow *window, T *&outPointer)
    {
        outPointer = static_cast<T *>(glfwGetWindowUserPointer(window));
        return outPointer != nullptr;
    }

    void KeyCallback(GLFWwindow *window, int key, int, int action, int)
    {
        House2D *house = nullptr;
        if (TryGetGLWindowUserPointer(window, house))
            house->HandleKey(key, action);
    }

    // Gets called when the window is resized and updates the OpenGL viewport accordingly.
    void FramebufferSizeCallback(GLFWwindow *, int width, int height)
    {
        glViewport(0, 0, width, height);
    }

    void CheckOpenGLErrors()
    {
        for (GLenum error = glGetError(); error != GL_NO_ERROR; error = glGetError())
            std::fprintf(stderr, "OpenGL error: 0x%X\n", error);
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

    const GLuint shaderProgram = CreateShaderProgram();
    if (shaderProgram == 0)
    {
        glfwDestroyWindow(window);
        glfwTerminate();
        return EXIT_FAILURE;
    }

    House2D house(shaderProgram);
    glfwSetWindowUserPointer(window, &house);
    glfwSetKeyCallback(window, KeyCallback);

    // Define the vertices of a triangle in normalized device coordinates.
    constexpr float triangleShape[] = {
            0.0f, 0.0f,
            1.0f, 0.0f,
            1.0f, 1.0f };

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

        house.Draw();

        CheckOpenGLErrors();
        glfwSwapBuffers(window); // Swap the front(read) and back(write) buffers to display the rendered image.
        glfwPollEvents();        // Poll for events such as keyboard and mouse input.
    }

    // Cleanup resources before exiting.
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(shaderProgram);
    glfwDestroyWindow(window);
    glfwTerminate();
    return EXIT_SUCCESS;
}
