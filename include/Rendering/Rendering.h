#pragma once

#include "Shared/Shared.h"

#include <cstdio>
#include <cstddef>
#include <string>
#include <type_traits>

#define DEFAULT_ASPECT_RATIO Rendering::DefaultAspectRatio

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
    
    int SetupWindow(GLFWwindow *&outWindow, const int width = 800, const int height = 600, const char *title = "Engine Window", const int swapInterval = 1);
}
