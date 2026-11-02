#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>

namespace
{
    struct Uniforms
    {
        GLint position;
        GLint scale;
        GLint rotation;
        GLint color;
    };

    struct Color
    {
        float r;
        float g;
        float b;
    };

    bool roof_color_changed = false;

    bool LoadShaderSource(const char *filename, std::string &source)
    {
        const std::string path = std::string(ENGINE_SHADER_DIR) + "/" + filename;
        std::ifstream file(path);
        if (!file)
        {
            std::fprintf(stderr, "Failed to open shader file: %s\n", path.c_str());
            return false;
        }

        std::ostringstream contents;
        contents << file.rdbuf();
        if (file.bad())
        {
            std::fprintf(stderr, "Failed to read shader file: %s\n", path.c_str());
            return false;
        }

        source = contents.str();
        return true;
    }

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

    GLuint CreateShaderProgram()
    {
        std::string vertex_source;
        std::string fragment_source;
        if (!LoadShaderSource("vertex.glsl", vertex_source) ||
            !LoadShaderSource("fragment.glsl", fragment_source))
        {
            return 0;
        }

        const GLuint vertex_shader = CompileShader(GL_VERTEX_SHADER, vertex_source.c_str());
        const GLuint fragment_shader = CompileShader(GL_FRAGMENT_SHADER, fragment_source.c_str());
        if (vertex_shader == 0 || fragment_shader == 0)
        {
            glDeleteShader(vertex_shader);
            glDeleteShader(fragment_shader);
            return 0;
        }

        const GLuint program = glCreateProgram();
        glAttachShader(program, vertex_shader);
        glAttachShader(program, fragment_shader);
        glLinkProgram(program);
        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);

        GLint success = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if (success == GL_FALSE)
        {
            char log[512]{};
            glGetProgramInfoLog(program, sizeof(log), nullptr, log);
            std::fprintf(stderr, "Shader link failed: %s\n", log);
            glDeleteProgram(program);
            return 0;
        }

        return program;
    }

    void DrawTriangle(const Uniforms &uniforms, float x, float y, float width, float height,
                      float rotation, Color color)
    {
        glUniform2f(uniforms.position, x, y);
        glUniform2f(uniforms.scale, width, height);
        glUniform1f(uniforms.rotation, rotation);
        glUniform3f(uniforms.color, color.r, color.g, color.b);
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }

    void DrawRectangle(const Uniforms &uniforms, float x, float y, float width, float height,
                       Color color)
    {
        DrawTriangle(uniforms, x, y, width, height, 0.0f, color);
        DrawTriangle(uniforms, x + width, y + height, -width, -height, 0.0f, color);
    }

    void DrawHouse(const Uniforms &uniforms)
    {
        constexpr Color wall{0.82f, 0.55f, 0.30f};
        constexpr Color roof{0.75f, 0.28f, 0.20f};
        constexpr Color changed_roof{0.20f, 0.45f, 0.85f};
        constexpr Color door{0.35f, 0.19f, 0.12f};
        constexpr Color window{0.35f, 0.75f, 0.88f};

        DrawRectangle(uniforms, -0.55f, -0.75f, 1.10f, 0.87f, wall);
        DrawTriangle(uniforms, -0.70f, 0.08f, 0.70f, 0.68f, 0.0f,
                     roof_color_changed ? changed_roof : roof);
        DrawTriangle(uniforms, 0.70f, 0.08f, -0.70f, 0.68f, 0.0f,
                     roof_color_changed ? changed_roof : roof);
        DrawRectangle(uniforms, -0.14f, -0.75f, 0.28f, 0.47f, door);
        DrawRectangle(uniforms, -0.46f, -0.02f, 0.21f, 0.18f, window);
        DrawRectangle(uniforms, 0.25f, -0.02f, 0.21f, 0.18f, window);
    }

    void KeyCallback(GLFWwindow *window, int key, int, int action, int)
    {
        if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
        {
            roof_color_changed = !roof_color_changed;
        }

        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
    }

    void FramebufferSizeCallback(GLFWwindow *, int width, int height)
    {
        glViewport(0, 0, width, height);
    }

    void CheckOpenGLErrors()
    {
        for (GLenum error = glGetError(); error != GL_NO_ERROR; error = glGetError())
        {
            std::fprintf(stderr, "OpenGL error: 0x%X\n", error);
        }
    }
}

int main()
{
    glfwSetErrorCallback([](int error, const char *description)
                         { std::fprintf(stderr, "GLFW error %d: %s\n", error, description); });

    if (glfwInit() != GLFW_TRUE)
        return EXIT_FAILURE;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window = glfwCreateWindow(1280, 720, "Engine", nullptr, nullptr);
    if (window == nullptr)
    {
        glfwTerminate();
        return EXIT_FAILURE;
    }

    glfwMakeContextCurrent(window);
    if (gladLoadGL(reinterpret_cast<GLADloadfunc>(glfwGetProcAddress)) == 0)
    {
        std::fputs("Failed to load OpenGL functions with GLAD.\n", stderr);
        glfwDestroyWindow(window);
        glfwTerminate();
        return EXIT_FAILURE;
    }

    glfwSetKeyCallback(window, KeyCallback);
    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    FramebufferSizeCallback(window, width, height);
    glfwSwapInterval(1);

    const GLuint program = CreateShaderProgram();
    if (program == 0)
    {
        glfwDestroyWindow(window);
        glfwTerminate();
        return EXIT_FAILURE;
    }

    const Uniforms uniforms = {
        glGetUniformLocation(program, "position"),
        glGetUniformLocation(program, "scale"),
        glGetUniformLocation(program, "rotation"),
        glGetUniformLocation(program, "color")};

    constexpr float triangle[] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f};

    GLuint vao = 0;
    GLuint vbo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(triangle), triangle, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);

    while (glfwWindowShouldClose(window) == GLFW_FALSE)
    {
        glClearColor(0.08f, 0.12f, 0.18f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(program);
        glBindVertexArray(vao);
        DrawHouse(uniforms);
        CheckOpenGLErrors();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(program);
    glfwDestroyWindow(window);
    glfwTerminate();
    return EXIT_SUCCESS;
}
