#pragma once

#include "Shared/Shared.h"

#include <cstdio>
#include <cstddef>
#include <string>

#define DEFAULT_ASPECT_RATIO Rendering::DefaultAspectRatio
#define ASPECT_RATIO(window) Rendering::GetWindowAspectRatio(window)

namespace Rendering
{
    inline constexpr float DefaultAspectRatio = 16.0f / 9.0f;

    enum ShaderType : GLenum
    {
        VertexShader = GL_VERTEX_SHADER,
        FragmentShader = GL_FRAGMENT_SHADER,
        GeometryShader = GL_GEOMETRY_SHADER
    };

    struct ShaderSource
    {
        const std::string source;
        const GLenum type;
        const bool isFile = false;
    };

    bool LoadShaderSource(const char *filename, std::string &outSource);
    bool CompileShader(GLenum type, const char *source, ResourceHandle &outShader);
    bool LinkShaders(ResourceHandle &outShaderProgram, const ResourceHandle *shaders, std::size_t shadersCount);
    bool CreateShaderProgram(ResourceHandle &outShaderProgram, const ShaderSource *sources, std::size_t sourcesCount);

    [[nodiscard]] float GetWindowAspectRatio(GLFWwindow &window);
    int SetupWindow(GLFWwindow *&outWindow, const int width = 800, const int height = 600, const char *title = "Engine Window");

    template <typename T>
    bool TryGetWindowUserPointer(GLFWwindow &window, T *&outPointer)
    {
        outPointer = static_cast<T *>(glfwGetWindowUserPointer(&window));
        return outPointer != nullptr;
    }
}
